#include "conversions.hpp"
#include "nifloader/collision_object_loader_state.hpp"
#include "nifloader/loader.hpp"
#include "nifloader/nif_resource_manager.hpp"
#include "nifloader/mesh_loader_state.hpp"
#include "nifloader/scene.hpp"
#include "ogrebullet/collision_object_manager.hpp"
#include <boost/graph/depth_first_search.hpp>
#include <set>

namespace oo {

namespace {

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

  explicit NifVisitor(std::string name, std::string group,
                      gsl::not_null<Ogre::SceneManager *> scnMgr,
                      gsl::not_null<btDiscreteDynamicsWorld *> world,
                      gsl::not_null<Ogre::SceneNode *> root)
      : mName(std::move(name)), mGroup(std::move(group)),
        mScnMgr(scnMgr), mWorld(world), mRoot(root) {}

  gsl::not_null<Ogre::SceneNode *> getRoot() const noexcept {
    return mRoot;
  }

 private:
  void discover_vertex(const nif::NiNode &node, const Graph &g);
  void discover_vertex(const nif::bhk::CollisionObject &node,
                       vertex_descriptor v, const Graph &g);
  void discover_vertex(const nif::NiTriBasedGeom &node,
                       vertex_descriptor v, const Graph &g);

  void finish_vertex(const nif::NiNode &node, const Graph &g);

  std::string mName;
  std::string mGroup;
  bool mHasHavok{false};
  bool mIsSkeleton{false};

  gsl::not_null<Ogre::SceneManager *> mScnMgr;
  gsl::not_null<btDiscreteDynamicsWorld *> mWorld;
  gsl::not_null<Ogre::SceneNode *> mRoot;
};

}

Ogre::SceneNode *insertNif(const std::string &name, const std::string &group,
                           gsl::not_null<Ogre::SceneManager *> scnMgr,
                           gsl::not_null<btDiscreteDynamicsWorld *> world,
                           gsl::not_null<Ogre::SceneNode *> parent) {
  auto nifPtr{Ogre::NifResourceManager::getSingleton().getByName(name, group)};
  if (!nifPtr) return nullptr;
  auto graph{nifPtr->getBlockGraph()};

  // TODO: Call dfs
  std::vector<boost::default_color_type> colorMap(boost::num_vertices(graph));
  const auto propertyMap{boost::make_iterator_property_map(
      colorMap.begin(), boost::get(boost::vertex_index, graph))};
  NifVisitor visitor(name, group, scnMgr, world, parent);
  boost::depth_first_search(graph, visitor, propertyMap);

  return visitor.getRoot();
}

Ogre::SceneNode *insertNif(const std::string &name, const std::string &group,
                           gsl::not_null<Ogre::SceneManager *> scnMgr,
                           gsl::not_null<btDiscreteDynamicsWorld *> world) {
  return oo::insertNif(name, group, scnMgr, world,
                       gsl::make_not_null(scnMgr->getRootSceneNode()));
}

void NifVisitor::start_vertex(vertex_descriptor, const Graph &g) {
  // Look for a BSXFlags
  auto it{std::find_if(g.vertex_set().begin(), g.vertex_set().end(),
                       [&g](vertex_descriptor v) -> bool {
                         return dynamic_cast<const nif::BSXFlags *>(&*g[v]);
                       })};
  if (it != g.vertex_set().end()) {
    const auto &bsxFlags{static_cast<const nif::BSXFlags &>(*g[*it])};
    using Flags = nif::BSXFlags::Flags;
    const Flags flags{bsxFlags.data};
    if ((flags & Flags::bHavok) != Flags::bNone) mHasHavok = true;
    if ((flags & Flags::bRagdoll) != Flags::bNone) mIsSkeleton = true;
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
  if (!mIsSkeleton) {
    std::string name{node.name.str()};
    //C++20: if (name.starts_with("Bip01")
    if (name.substr(0, 5u) == "Bip01") return;
  }

  const Ogre::Vector3 tra{oo::fromBSCoordinates(oo::fromNif(node.translation))};
  const Ogre::Quaternion rot{oo::fromBSCoordinates(oo::fromNif(node.rotation))};
  if (mIsSkeleton) {
  }
  mRoot = gsl::make_not_null(mRoot->createChildSceneNode(tra, rot));
}

void NifVisitor::discover_vertex(const nif::bhk::CollisionObject &node,
                                 vertex_descriptor v,
                                 const Graph &g) {
  // Collision objects come in one piece. Dispatch to the collision loader,
  // starting from current working root.

  // We can't create a reloadable resource because the loader requires state.
  auto &colObjMgr{Ogre::CollisionObjectManager::getSingleton()};
  const std::string name{mName + std::to_string(v) + "CollisionObject"};
  auto[collisionObjectPtr, created]{colObjMgr.createOrRetrieve(
      name, mGroup, true, nullptr)};
  if (created) {
    oo::CollisionObjectLoaderState loader(
        static_cast<Ogre::CollisionObject *>(collisionObjectPtr.get()), g, v,
        mHasHavok, mIsSkeleton);
  }

  Ogre::RigidBody *rigidBody = [&]() -> Ogre::RigidBody * {
    const std::map<std::string, std::string> params{
        {"collisionObject", name},
        {"resourceGroup", mGroup}
    };

    // Yes, we are using an exception for control flow. It is necessary, see
    // RigidBodyFactory::createInstanceImpl.
    // TODO: Replace with a mgr->createRigidBody on a derived SceneManager
    try {
      return dynamic_cast<Ogre::RigidBody *>(
          mScnMgr->createMovableObject("RigidBody", &params));
    } catch (const Ogre::PartialCollisionObjectException &) {
      return nullptr;
    }
  }();

  if (!rigidBody) return;

  mRoot->attachObject(rigidBody);
  // TODO: Replace with rigidBody->attach(world)
  mWorld->addRigidBody(rigidBody->getRigidBody());
}

void NifVisitor::discover_vertex(const nif::NiTriBasedGeom &node,
                                 vertex_descriptor v, const Graph &g) {
  auto &meshMgr{Ogre::MeshManager::getSingleton()};
  const std::string name{mName + std::to_string(v) + "Mesh"};
  auto[ptr, created]{meshMgr.createOrRetrieve(
      name, mGroup, true, nullptr)};
  Ogre::MeshPtr meshPtr{std::static_pointer_cast<Ogre::Mesh>(ptr)};
  if (created) {
    oo::MeshLoaderState loader(meshPtr.get(), g, v);
  }

  Ogre::Entity *entity{mScnMgr->createEntity(meshPtr)};
  if (!entity) return;

  mRoot->attachObject(entity);
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

void NifVisitor::finish_vertex(const nif::NiNode &node, const Graph &g) {
  if (auto *parent{mRoot->getParentSceneNode()}; parent) {
    mRoot = gsl::make_not_null(parent);
  }
}

} // namespace oo