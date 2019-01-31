#ifndef OPENOBLIVION_NIF_COLLISION_OBJECT_LOADER_STATE_HPP
#define OPENOBLIVION_NIF_COLLISION_OBJECT_LOADER_STATE_HPP

#include "nifloader/loader.hpp"
#include "nifloader/loader_state.hpp"
#include "ogrebullet/collision_object.hpp"
#include "ogrebullet/rigid_body.hpp"
#include "settings.hpp"
#include <Ogre.h>
#include <spdlog/spdlog.h>

namespace oo {

/// \addtogroup OpenOblivionNifloader
/// @{

class CollisionObjectLoaderState {
 public:
  CollisionObjectLoaderState(Ogre::CollisionObject *collisionObject,
                             oo::BlockGraph blocks);
};

struct CollisionObjectVisitor {
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

  explicit CollisionObjectVisitor(Ogre::CollisionObject *rigidBody)
      : mRigidBody(rigidBody), mLogger(spdlog::get(oo::LOG)) {}

 private:
  Ogre::CollisionObject *mRigidBody{};
  Ogre::Matrix4 mTransform{Ogre::Matrix4::IDENTITY};
  bool mHasHavok{false};
  std::shared_ptr<spdlog::logger> mLogger{};

  void discover_vertex(const nif::NiNode &node, const Graph &g);
  void discover_vertex(const nif::BSXFlags &bsxFlags, const Graph &g);
  void discover_vertex(const nif::BSBound &bsBound, const Graph &g);
  void discover_vertex(const nif::bhk::CollisionObject &collisionObject,
                       const Graph &g);

  void finish_vertex(const nif::NiNode &node, const Graph &g);

  template<class U, class T>
  const U &getRef(const Graph &g, nif::basic::Ref<T> ref) {
    const auto refInt{static_cast<int32_t>(ref)};
    if (refInt < 0
        || static_cast<std::size_t>(refInt) >= boost::num_vertices(g)) {
      OGRE_EXCEPT(Ogre::Exception::ERR_INTERNAL_ERROR,
                  "Nonexistent reference",
                  "CollisionObjectVisitor");
    }
    return dynamic_cast<const U &>(*g[refInt]);
  }

  void parseCollisionObject(const Graph &g,
                            const nif::bhk::CollisionObject &block);

  std::pair<std::unique_ptr<btCollisionShape>,
            std::unique_ptr<Ogre::RigidBodyInfo>>
  parseWorldObject(const Graph &g, const nif::bhk::WorldObject &block);

  Ogre::RigidBodyInfo
  generateRigidBodyInfo(const nif::bhk::RigidBody &block) const;

  std::unique_ptr<btCollisionShape>
  parseShape(const Graph &g, const nif::bhk::Shape &block);

  std::unique_ptr<btCollisionShape>
  parseShape(const Graph &g, const nif::bhk::MoppBvTreeShape &shape);

  std::unique_ptr<btCollisionShape>
  parseShape(const Graph &g, const nif::bhk::PackedNiTriStripsShape &shape);

  std::unique_ptr<btCollisionShape>
  parseShape(const Graph &g, const nif::bhk::ConvexVerticesShape &shape);

  std::unique_ptr<btCollisionShape>
  parseShape(const Graph &g, const nif::bhk::BoxShape &shape);

  std::unique_ptr<btCollisionShape>
  parseNiTriStripsData(const Graph &g,
                       const nif::hk::PackedNiTriStripsData &block);

  // Fill indexBuf with the indexed triangle data of block and return a pointer
  // to the first byte. indexBuf will be resized if necessary.
  unsigned char *fillIndexBuffer(std::vector<uint16_t> &indexBuf,
                                 const nif::hk::PackedNiTriStripsData &block);

  // Fill vertexBuf with the vertex data of block and return a pointer to the
  // first byte. vertexBuf will be resized if necessary.
  unsigned char *fillVertexBuffer(std::vector<float> &vertexBuf,
                                  const nif::hk::PackedNiTriStripsData &block);
};

Ogre::Matrix4 getRigidBodyTransform(const nif::bhk::RigidBodyT &body);

///@}

} // namespace oo

#endif // OPENOBLIVION_NIF_COLLISION_OBJECT_LOADER_STATE_HPP
