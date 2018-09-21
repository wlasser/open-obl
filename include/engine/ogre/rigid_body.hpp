#ifndef OPENOBLIVION_ENGINE_RIGID_BODY_HPP
#define OPENOBLIVION_ENGINE_RIGID_BODY_HPP

#include "engine/nifloader/loader_state.hpp"
#include "nif/bhk.hpp"
#include <btBulletDynamicsCommon.h>
#include <OgreResource.h>
#include <memory>

namespace Ogre {

class RigidBody : public Ogre::Resource {
 public:
  RigidBody(ResourceManager *creator,
            const String &name,
            ResourceHandle handle,
            const String &group,
            bool isManual = false,
            ManualResourceLoader *loader = nullptr);

  ~RigidBody() override {
    unload();
  }

  btRigidBody *getRigidBody();

 protected:
  void loadImpl() override;
  void unloadImpl() override;

 private:
  friend struct RigidBodyNifVisitor;
  // TODO: Use custom allocators
  std::unique_ptr<btRigidBody> mRigidBody{};
  std::unique_ptr<btCollisionShape> mCollisionShape{};
};

struct RigidBodyNifVisitor {
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

  explicit RigidBodyNifVisitor(RigidBody *rigidBody) : mRigidBody(rigidBody) {}

 private:
  Ogre::Matrix4 mTransform{Ogre::Matrix4::IDENTITY};
  RigidBody *mRigidBody{};
  bool mHasHavok{false};

  void parseCollisionObject(const Graph &g,
                            nif::bhk::CollisionObject *block,
                            engine::nifloader::LoadStatus &tag);

  void parseWorldObject(const Graph &g,
                        nif::bhk::WorldObject *block,
                        engine::nifloader::LoadStatus &tag);

  std::unique_ptr<btCollisionShape>
  parseShape(const Graph &g,
             nif::bhk::Shape *block,
             engine::nifloader::LoadStatus &tag);

  std::unique_ptr<btCollisionShape>
  parseNiTriStripsData(const Graph &g,
                       nif::hk::PackedNiTriStripsData *block,
                       engine::nifloader::LoadStatus &tag);
};

using RigidBodyPtr = std::shared_ptr<RigidBody>;

} // namespace Ogre

#endif // OPENOBLIVION_ENGINE_RIGID_BODY_HPP
