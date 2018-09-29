#ifndef OPENOBLIVION_ENGINE_NIF_COLLISION_OBJECT_LOADER_STATE_HPP
#define OPENOBLIVION_ENGINE_NIF_COLLISION_OBJECT_LOADER_STATE_HPP

#include "engine/nifloader/loader.hpp"
#include "engine/nifloader/loader_state.hpp"
#include "nif/bhk.hpp"
#include "ogrebullet/collision_object.hpp"
#include "ogrebullet/rigid_body.hpp"
#include "engine/settings.hpp"
#include <Ogre.h>
#include <spdlog/spdlog.h>

namespace engine::nifloader {

class CollisionObjectLoaderState {
 public:
  CollisionObjectLoaderState(Ogre::CollisionObject *collisionObject,
                             nifloader::BlockGraph untaggedBlocks);
};

struct CollisionObjectVisitor {
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

  explicit CollisionObjectVisitor(Ogre::CollisionObject *rigidBody)
      : mRigidBody(rigidBody), mLogger(spdlog::get(engine::settings::log)) {}

 private:
  Ogre::Matrix4 mTransform{Ogre::Matrix4::IDENTITY};
  Ogre::CollisionObject *mRigidBody{};
  bool mHasHavok{false};
  std::shared_ptr<spdlog::logger> mLogger{};

  template<class T>
  struct RefResult {
    T *block;
    engine::nifloader::LoadStatus &tag;
  };

  template<class U, class T>
  RefResult<U> getRef(const Graph &g, nif::basic::Ref<T> ref) {
    auto refInt = static_cast<__int32_t>(ref);
    if (refInt < 0 || refInt >= boost::num_vertices(g)) {
      OGRE_EXCEPT(Ogre::Exception::ERR_INTERNAL_ERROR,
                  "Nonexistent reference",
                  "CollisionObjectVisitor");
    }
    auto &taggedBlock = g[refInt];
    auto *block = dynamic_cast<U *>(taggedBlock.block.get());
    if (block == nullptr) {
      OGRE_EXCEPT(Ogre::Exception::ERR_INTERNAL_ERROR,
                  "Nonexistent reference",
                  "CollisionObjectVisitor");
    }
    return {block, taggedBlock.tag};
  }

  void parseCollisionObject(const Graph &g,
                            nif::bhk::CollisionObject *block,
                            engine::nifloader::LoadStatus &tag);

  std::pair<std::unique_ptr<btCollisionShape>,
            std::unique_ptr<Ogre::RigidBodyInfo>>
  parseWorldObject(const Graph &g,
                   nif::bhk::WorldObject *block,
                   engine::nifloader::LoadStatus &tag);

  Ogre::RigidBodyInfo generateRigidBodyInfo(nif::bhk::RigidBody *block) const;

  std::unique_ptr<btCollisionShape>
  parseShape(const Graph &g,
             nif::bhk::Shape *block,
             engine::nifloader::LoadStatus &tag);

  std::unique_ptr<btCollisionShape>
  parseNiTriStripsData(const Graph &g,
                       nif::hk::PackedNiTriStripsData *block,
                       engine::nifloader::LoadStatus &tag);
};

} // namespace engine::nifloader

#endif // OPENOBLIVION_ENGINE_NIF_COLLISION_OBJECT_LOADER_STATE_HPP
