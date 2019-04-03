#include "bullet/collision.hpp"
#include "conversions.hpp"
#include "entity.hpp"
#include "mesh_manager.hpp"
#include "nifloader/collision_object_loader_state.hpp"
#include "nifloader/loader.hpp"
#include "nifloader/nif_resource_manager.hpp"
#include "nifloader/mesh_loader_state.hpp"
#include "nifloader/scene.hpp"
#include "ogrebullet/collision_shape_manager.hpp"
#include <boost/graph/depth_first_search.hpp>
#include <OgreTagPoint.h>
#include <set>

namespace oo {

namespace {

struct NifVisitorState {
  std::string mName;
  std::string mGroup;
  bool mHasHavok{false};
  bool mIsSkeleton{false};
  /// Keep track of the NiNode blocks which have had their child geometry nodes
  /// processed.
  std::set<oo::BlockGraph::vertex_descriptor> mVisitedGeometry{};

  gsl::not_null<Ogre::SceneManager *> mScnMgr;
  gsl::not_null<btDiscreteDynamicsWorld *> mWorld;
  gsl::not_null<Ogre::SceneNode *> mRoot;
  gsl::not_null<Ogre::SceneNode *> mParent;

  explicit NifVisitorState(std::string name, std::string group,
                           gsl::not_null<Ogre::SceneManager *> scnMgr,
                           gsl::not_null<btDiscreteDynamicsWorld *> world,
                           gsl::not_null<Ogre::SceneNode *> root)
      : mName(std::move(name)), mGroup(std::move(group)),
        mScnMgr(scnMgr), mWorld(world), mRoot(root), mParent(root) {}
};

class NifVisitor {
 public:
  using Graph = BlockGraph;
  using vertex_descriptor = Graph::vertex_descriptor;
  using edge_descriptor = Graph::edge_descriptor;

  void start_vertex(vertex_descriptor v, const Graph &g);
  void discover_vertex(vertex_descriptor v, const Graph &g);
  void finish_vertex(vertex_descriptor v, const Graph &g);

  [[maybe_unused]] void initialize_vertex(vertex_descriptor, const Graph &) {}
  [[maybe_unused]] void examine_edge(edge_descriptor, const Graph &) {}
  [[maybe_unused]] void tree_edge(edge_descriptor, const Graph &) {}
  [[maybe_unused]] void back_edge(edge_descriptor, const Graph &) {}
  [[maybe_unused]] void forward_or_cross_edge(edge_descriptor, const Graph &) {}
  [[maybe_unused]] void finish_edge(edge_descriptor, const Graph &) {}

  explicit NifVisitor(NifVisitorState *state) : mState(state) {}

 private:
  void discover_vertex(const nif::NiNode &node, const Graph &g);
  void discover_vertex(const nif::bhk::CollisionObject &node,
                       vertex_descriptor v, const Graph &g);
  void discover_vertex(const nif::NiTriBasedGeom &node,
                       vertex_descriptor v, const Graph &g);

  void finish_vertex(const nif::NiNode &node, const Graph &g);

  NifVisitorState *mState;
};

struct RagdollVisitorState {
  std::string mName;
  std::string mGroup;
  bool mHasHavok{false};
  bool mIsSkeleton{false};

  gsl::not_null<Ogre::SceneManager *> mScnMgr;
  gsl::not_null<btDiscreteDynamicsWorld *> mWorld;
  gsl::not_null<oo::Entity *> mEntity;

  explicit RagdollVisitorState(std::string name, std::string group,
                               gsl::not_null<Ogre::SceneManager *> scnMgr,
                               gsl::not_null<btDiscreteDynamicsWorld *> world,
                               gsl::not_null<oo::Entity *> entity)
      : mName(std::move(name)), mGroup(std::move(group)),
        mScnMgr(scnMgr), mWorld(world), mEntity(entity) {}
};

class RagdollVisitor {
 public:
  using Graph = BlockGraph;
  using vertex_descriptor = Graph::vertex_descriptor;
  using edge_descriptor = Graph::edge_descriptor;

  void start_vertex(vertex_descriptor v, const Graph &g);
  void discover_vertex(vertex_descriptor v, const Graph &g);
  [[maybe_unused]] void finish_vertex(vertex_descriptor, const Graph &) {}

  [[maybe_unused]] void initialize_vertex(vertex_descriptor, const Graph &) {}
  [[maybe_unused]] void examine_edge(edge_descriptor, const Graph &) {}
  [[maybe_unused]] void tree_edge(edge_descriptor, const Graph &) {}
  [[maybe_unused]] void back_edge(edge_descriptor, const Graph &) {}
  [[maybe_unused]] void forward_or_cross_edge(edge_descriptor, const Graph &) {}
  [[maybe_unused]] void finish_edge(edge_descriptor, const Graph &) {}

  explicit RagdollVisitor(RagdollVisitorState *state) : mState(state) {}

 private:
  void discover_vertex(const nif::bhk::BlendCollisionObject &node,
                       vertex_descriptor v, const Graph &g);

  RagdollVisitorState *mState;
};


//===----------------------------------------------------------------------===//
// start_vertex
//===----------------------------------------------------------------------===//

void NifVisitor::start_vertex(vertex_descriptor, const Graph &g) {
  const auto &vertices{g.vertex_set()};
  auto it{std::find_if(vertices.begin(), vertices.end(), [&g](auto v) -> bool {
    return dynamic_cast<const nif::BSXFlags *>(&*g[v]);
  })};
  if (it != vertices.end()) {
    const auto &bsxFlags{static_cast<const nif::BSXFlags &>(*g[*it])};
    using Flags = nif::BSXFlags::Flags;
    const Flags flags{bsxFlags.data};
    if ((flags & Flags::bHavok) != Flags::bNone) mState->mHasHavok = true;
    if ((flags & Flags::bRagdoll) != Flags::bNone) mState->mIsSkeleton = true;
  }
}

void RagdollVisitor::start_vertex(vertex_descriptor, const Graph &g) {
  // TODO: Reduce duplication
  const auto &vertices{g.vertex_set()};
  auto it{std::find_if(vertices.begin(), vertices.end(), [&g](auto v) -> bool {
    return dynamic_cast<const nif::BSXFlags *>(&*g[v]);
  })};
  if (it != vertices.end()) {
    const auto &bsxFlags{static_cast<const nif::BSXFlags &>(*g[*it])};
    using Flags = nif::BSXFlags::Flags;
    const Flags flags{bsxFlags.data};
    if ((flags & Flags::bHavok) != Flags::bNone) mState->mHasHavok = true;
    if ((flags & Flags::bRagdoll) != Flags::bNone) mState->mIsSkeleton = true;
  }
}

//===----------------------------------------------------------------------===//
// discover_vertex
//===----------------------------------------------------------------------===//

void NifVisitor::discover_vertex(vertex_descriptor v, const Graph &g) {
  const auto &block{*g[v]};

  if (dynamic_cast<const nif::NiNode *>(&block)) {
    discover_vertex(static_cast<const nif::NiNode &>(block), g);
  } else if (dynamic_cast<const nif::bhk::CollisionObject *>(&block)) {
    discover_vertex(static_cast<const nif::bhk::CollisionObject &>(block),
                    v, g);
  } else if (dynamic_cast<const nif::NiTriBasedGeom *>(&block)) {
    discover_vertex(static_cast<const nif::NiTriBasedGeom &>(block), v, g);
  }
}

void NifVisitor::discover_vertex(const nif::NiNode &node, const Graph &) {
  // If we are not in a skeleton then skeletal nodes do not need to be added
  // as scene nodes, they only need to present in the nif file when processing
  // the mesh. This is purely an optimization, and there is no way for sure to
  // know if a node is skeletal or not.
  if (!mState->mIsSkeleton) {
    const std::string name{node.name.str()};
    //C++20: if (name.starts_with("Bip01")
    if (name.substr(0, 5u) == "Bip01") return;
  }

  const Ogre::Vector3 tra{oo::fromBSCoordinates(node.translation)};
  const Ogre::Quaternion rot{oo::fromBSCoordinates(node.rotation)};
  if (mState->mIsSkeleton) {
  }
  mState->mRoot = gsl::make_not_null(mState->mRoot->createChildSceneNode());
  mState->mRoot->setPosition(tra);
  // TODO: I don't know why this needs to be inverted, I can only imagine
  //       somewhere there is a bug, but I cannot find it. If you can work it
  //       out, dear reader, please tell me...
  mState->mRoot->rotate(rot.Inverse(), Ogre::SceneNode::TS_WORLD);
  mState->mRoot->setInitialState();
}

void NifVisitor::discover_vertex(const nif::bhk::CollisionObject &,
                                 vertex_descriptor v,
                                 const Graph &g) {
  const vertex_descriptor u{boost::in_edges(v, g).first->m_source};

  // We can't create a reloadable resource because the loader requires state.
  auto &colObjMgr{Ogre::CollisionShapeManager::getSingleton()};
  const std::string name{mState->mName + std::to_string(v) + "CollisionShape"};
  const std::string &group{mState->mGroup};
  auto[ptr, created]{colObjMgr.createOrRetrieve(name, group, true, nullptr)};
  Ogre::CollisionShapePtr collisionShapePtr
      {std::static_pointer_cast<Ogre::CollisionShape>(ptr)};
  if (created) oo::createCollisionObject(collisionShapePtr.get(), u, g);

  Ogre::RigidBody *rigidBody = [&]() -> Ogre::RigidBody * {
    const std::map<std::string, std::string> params{
        {"collisionShape", name},
        {"resourceGroup", mState->mGroup}
    };

    // Yes, we are using an exception for control flow. It is necessary, see
    // RigidBodyFactory::createInstanceImpl.
    // TODO: Replace with a mgr->createRigidBody on a derived SceneManager
    try {
      return dynamic_cast<Ogre::RigidBody *>(
          mState->mScnMgr->createMovableObject("RigidBody", &params));
    } catch (const Ogre::PartialCollisionObjectException &) {
      return nullptr;
    }
  }();

  if (!rigidBody) return;

  mState->mRoot->attachObject(rigidBody);
  // TODO: Replace with rigidBody->attach(world)
  bullet::addRigidBody(mState->mWorld, gsl::make_not_null(rigidBody));
}

void NifVisitor::discover_vertex(const nif::NiTriBasedGeom &,
                                 vertex_descriptor v, const Graph &g) {
  // All sibling NiTriBasedGeom blocks are submeshes of a mesh owned by the
  // parent NiNode.
  const vertex_descriptor u{boost::in_edges(v, g).first->m_source};

  //C++20: if (mState->mVisitedGeometry.contains(u)) return;
  if (mState->mVisitedGeometry.count(u) > 0) return;

  auto &meshMgr{oo::MeshManager::getSingleton()};
  const std::string name{mState->mName + std::to_string(u) + "Mesh"};
  const std::string &group{mState->mGroup};
  auto[ptr, created]{meshMgr.createOrRetrieve(name, group, true, nullptr)};
  oo::MeshPtr meshPtr{std::static_pointer_cast<oo::Mesh>(ptr)};
  if (created) oo::createMesh(meshPtr.get(), u, g);
  // Record that this node and its siblings have been visited.
  mState->mVisitedGeometry.emplace(u);

  Ogre::NameValuePairList params{{"mesh", meshPtr->getName()},
                                 {"resourceGroup", meshPtr->getGroup()}};
  auto *entity{dynamic_cast<oo::Entity *>(
                   mState->mScnMgr->createMovableObject("oo::Entity",
                                                        &params))};
  if (!entity) return;

  mState->mRoot->attachObject(entity);
}

void RagdollVisitor::discover_vertex(vertex_descriptor v, const Graph &g) {
  const auto &block{*g[v]};

  if (dynamic_cast<const nif::bhk::BlendCollisionObject *>(&block)) {
    discover_vertex(static_cast<const nif::bhk::BlendCollisionObject &>(block),
                    v, g);
  }
}

void RagdollVisitor::discover_vertex(const nif::bhk::BlendCollisionObject &node,
                                     vertex_descriptor v,
                                     const Graph &g) {
  // TODO: Reduce duplication
  const vertex_descriptor u{boost::in_edges(v, g).first->m_source};

  // We can't create a reloadable resource because the loader requires state.
  auto &colObjMgr{Ogre::CollisionShapeManager::getSingleton()};
  const std::string name{mState->mName + std::to_string(v) + "CollisionShape"};
  const std::string &group{mState->mGroup};
  auto[ptr, created]{colObjMgr.createOrRetrieve(name, group, true, nullptr)};
  Ogre::CollisionShapePtr collisionShapePtr
      {std::static_pointer_cast<Ogre::CollisionShape>(ptr)};
  if (created) oo::createCollisionObject(collisionShapePtr.get(), u, g);

  Ogre::RigidBody *rigidBody = [&]() -> Ogre::RigidBody * {
    const std::map<std::string, std::string> params{
        {"collisionShape", name},
        {"resourceGroup", mState->mGroup}
    };

    // Yes, we are using an exception for control flow. It is necessary, see
    // RigidBodyFactory::createInstanceImpl.
    // TODO: Replace with a mgr->createRigidBody on a derived SceneManager
    try {
      return dynamic_cast<Ogre::RigidBody *>(
          mState->mScnMgr->createMovableObject("RigidBody", &params));
    } catch (const Ogre::PartialCollisionObjectException &) {
      return nullptr;
    }
  }();

  if (!rigidBody) return;

  const auto &target{oo::getBlock<nif::NiNode>(g, node.target)};
  bullet::addRigidBody(mState->mWorld, gsl::make_not_null(rigidBody));
  auto *tp{mState->mEntity->attachObjectToBone(target.name.str(), rigidBody)};
  tp->setInheritOrientation(true);
}

//===----------------------------------------------------------------------===//
// finish_vertex
//===----------------------------------------------------------------------===//

void NifVisitor::finish_vertex(vertex_descriptor v, const Graph &g) {
  const auto &block{*g[v]};

  if (dynamic_cast<const nif::NiNode *>(&block)) {
    finish_vertex(static_cast<const nif::NiNode &>(block), g);
  }
}

void NifVisitor::finish_vertex(const nif::NiNode &node, const Graph &) {
  if (!mState->mIsSkeleton) {
    const std::string name{node.name.str()};
    //C++20: if (name.starts_with("Bip01")
    if (name.substr(0, 5u) == "Bip01") return;
  }

  if (auto *parent{mState->mRoot->getParentSceneNode()};
      parent && parent != mState->mParent) {
    mState->mRoot = gsl::make_not_null(parent);
  }
}

} // namespace

//===----------------------------------------------------------------------===//
// insertNif and Friends
//===----------------------------------------------------------------------===//

Ogre::SceneNode *insertNif(const std::string &name, const std::string &group,
                           gsl::not_null<Ogre::SceneManager *> scnMgr,
                           gsl::not_null<btDiscreteDynamicsWorld *> world,
                           gsl::not_null<Ogre::SceneNode *> parent) {
  auto nifPtr{Ogre::NifResourceManager::getSingleton().getByName(name, group)};
  if (!nifPtr) return nullptr;
  try {
    nifPtr->load();
  } catch (const std::exception &e) {
    spdlog::get(oo::LOG)->error("Nif load failed: {}", e.what());
    return nullptr;
  }
  auto graph{nifPtr->getBlockGraph()};

  std::vector<boost::default_color_type> colorMap(boost::num_vertices(graph));
  const auto propertyMap{boost::make_iterator_property_map(
      colorMap.begin(), boost::get(boost::vertex_index, graph))};
  NifVisitorState state(name, group, scnMgr, world, parent);
  boost::depth_first_search(graph, NifVisitor(&state), propertyMap);

  return state.mRoot;
}

Ogre::SceneNode *insertNif(const std::string &name, const std::string &group,
                           gsl::not_null<Ogre::SceneManager *> scnMgr,
                           gsl::not_null<btDiscreteDynamicsWorld *> world) {
  return oo::insertNif(name, group, scnMgr, world,
                       gsl::make_not_null(scnMgr->getRootSceneNode()));
}

void attachRagdoll(const std::string &name, const std::string &group,
                   gsl::not_null<Ogre::SceneManager *> scnMgr,
                   gsl::not_null<btDiscreteDynamicsWorld *> world,
                   gsl::not_null<oo::Entity *> entity) {
  auto nifPtr{Ogre::NifResourceManager::getSingleton().getByName(name, group)};
  if (!nifPtr) return;
  auto graph{nifPtr->getBlockGraph()};

  std::vector<boost::default_color_type> colorMap(boost::num_vertices(graph));
  const auto propertyMap{boost::make_iterator_property_map(
      colorMap.begin(), boost::get(boost::vertex_index, graph))};
  RagdollVisitorState state(name, group, scnMgr, world, entity);
  boost::depth_first_search(graph, RagdollVisitor(&state), propertyMap);
}

} // namespace oo