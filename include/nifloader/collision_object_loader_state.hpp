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

  explicit CollisionObjectLoaderState(Ogre::CollisionObject *collisionObject,
                                      Graph blocks);

 private:
  using CollisionShapeVector = std::vector<Ogre::CollisionShapePtr>;

  Ogre::CollisionObject *mRigidBody{};
  Ogre::Matrix4 mTransform{Ogre::Matrix4::IDENTITY};
  bool mHasHavok{false};
  bool mIsSkeleton{false};
  std::shared_ptr<spdlog::logger> mLogger{};

  void discover_vertex(const nif::NiNode &node, const Graph &g);
  void discover_vertex(const nif::BSXFlags &bsxFlags, const Graph &g);
  void discover_vertex(const nif::BSBound &bsBound, const Graph &g);
  void discover_vertex(const nif::bhk::CollisionObject &collisionObject,
                       const Graph &g);

  void finish_vertex(const nif::NiNode &node, const Graph &g);

  void parseCollisionObject(const Graph &g,
                            const nif::bhk::CollisionObject &block);

  std::pair<CollisionShapeVector, std::unique_ptr<Ogre::RigidBodyInfo>>
  parseWorldObject(const Graph &g, const nif::bhk::WorldObject &block);

  Ogre::RigidBodyInfo
  generateRigidBodyInfo(const nif::bhk::RigidBody &block) const;

  CollisionShapeVector
  parseShape(const Graph &g, const nif::bhk::Shape &block);

  CollisionShapeVector
  parseShape(const Graph &g, const nif::bhk::TransformShape &block);

  CollisionShapeVector
  parseShape(const Graph &g, const nif::bhk::CapsuleShape &block);

  CollisionShapeVector
  parseShape(const Graph &g, const nif::bhk::MoppBvTreeShape &shape);

  CollisionShapeVector
  parseShape(const Graph &g, const nif::bhk::ListShape &shape);

  CollisionShapeVector
  parseShape(const Graph &g, const nif::bhk::PackedNiTriStripsShape &shape);

  CollisionShapeVector
  parseShape(const Graph &g, const nif::bhk::ConvexVerticesShape &shape);

  CollisionShapeVector
  parseShape(const Graph &g, const nif::bhk::BoxShape &shape);

  CollisionShapeVector
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
