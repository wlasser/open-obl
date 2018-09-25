#ifndef OPENOBLIVION_ENGINE_COLLISION_OBJECT_HPP
#define OPENOBLIVION_ENGINE_COLLISION_OBJECT_HPP

#include "engine/nifloader/loader_state.hpp"
#include "engine/ogre/motion_state.hpp"
#include "engine/settings.hpp"
#include "nif/bhk.hpp"
#include <btBulletDynamicsCommon.h>
#include <OgreResource.h>
#include <spdlog/spdlog.h>
#include <memory>
#include <optional>

namespace Ogre {

using RigidBodyInfo = btRigidBody::btRigidBodyConstructionInfo;

class CollisionObject : public Ogre::Resource {
 public:
  CollisionObject(ResourceManager *creator,
                  const String &name,
                  ResourceHandle handle,
                  const String &group,
                  bool isManual = false,
                  ManualResourceLoader *loader = nullptr);

  ~CollisionObject() override {
    unload();
  }

  RigidBodyInfo *getRigidBodyInfo() const;

  btCollisionShape *getCollisionShape() const;

 protected:
  void loadImpl() override;
  void unloadImpl() override;

 private:
  friend struct CollisionObjectNifVisitor;

  std::unique_ptr<RigidBodyInfo> mInfo{};

  // For performance reasons we don't want to duplicate the collision shape
  // for multiple instances of the same entity. Ideally therefore this would be
  // a non-owning pointer into a central store, which would store the collision
  // shape along with any necessary buffers.
  // TODO: Centralise the collision shapes and make this non-owning
  std::unique_ptr<btCollisionShape> mCollisionShape{};

  // Necessary for mesh-based collision shapes, Bullet does not take ownership.
  std::vector<uint16_t> mIndexBuffer{};
  std::vector<float> mVertexBuffer{};
  std::unique_ptr<btStridingMeshInterface> mCollisionMesh{};
};

struct CollisionObjectNifVisitor {
  using Graph = engine::nifloader::TaggedBlockGraph;
  using vertex_descriptor = Graph::vertex_descriptor;
  using edge_descriptor = Graph::edge_descriptor;

  void initialize_vertex(vertex_descriptor v, const Graph &g) {}
  void start_vertex(vertex_descriptor v, const Graph &g);
  void discover_vertex(vertex_descriptor v, const Graph &g);
  void examine_edge(edge_descriptor e, const Graph &g) {}
  void tree_edge(edge_descriptor e, const Graph &g) {}
  void back_edge(edge_descriptor e, const Graph &g) {}
  void forward_or_cross_edge(edge_descriptor e, const Graph &g) {}
  void finish_edge(edge_descriptor e, const Graph &g) {}
  void finish_vertex(vertex_descriptor v, const Graph &g);

  explicit CollisionObjectNifVisitor(CollisionObject *rigidBody)
      : mRigidBody(rigidBody), mLogger(spdlog::get(engine::settings::log)) {}

 private:
  Ogre::Matrix4 mTransform{Ogre::Matrix4::IDENTITY};
  CollisionObject *mRigidBody{};
  bool mHasHavok{false};
  std::shared_ptr<spdlog::logger> mLogger{};

  template<class T>
  struct RefResult {
    T *block;
    engine::nifloader::LoadStatus &tag;
  };

  template<class U, class T>
  RefResult<U> getRef(const Graph &g, nif::basic::Ref<T> ref) {
    auto refInt = static_cast<int32_t>(ref);
    if (refInt < 0 || refInt >= boost::num_vertices(g)) {
      OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR,
                  "Nonexistent reference",
                  "CollisionObjectNifVisitor");
    }
    auto &taggedBlock = g[refInt];
    auto *block = dynamic_cast<U *>(taggedBlock.block.get());
    if (block == nullptr) {
      OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR,
                  "Nonexistent reference",
                  "CollisionObjectNifVisitor");
    }
    return {block, taggedBlock.tag};
  }

  void parseCollisionObject(const Graph &g,
                            nif::bhk::CollisionObject *block,
                            engine::nifloader::LoadStatus &tag);

  std::pair<std::unique_ptr<btCollisionShape>, std::unique_ptr<RigidBodyInfo>>
  parseWorldObject(const Graph &g,
                   nif::bhk::WorldObject *block,
                   engine::nifloader::LoadStatus &tag);

  RigidBodyInfo generateRigidBodyInfo(nif::bhk::RigidBody *block) const;

  std::unique_ptr<btCollisionShape>
  parseShape(const Graph &g,
             nif::bhk::Shape *block,
             engine::nifloader::LoadStatus &tag);

  std::unique_ptr<btCollisionShape>
  parseNiTriStripsData(const Graph &g,
                       nif::hk::PackedNiTriStripsData *block,
                       engine::nifloader::LoadStatus &tag);
};

using CollisionObjectPtr = std::shared_ptr<CollisionObject>;

} // namespace Ogre

#endif // OPENOBLIVION_ENGINE_COLLISION_OBJECT_HPP
