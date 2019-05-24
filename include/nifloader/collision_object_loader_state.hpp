#ifndef OPENOBL_NIF_COLLISION_OBJECT_LOADER_STATE_HPP
#define OPENOBL_NIF_COLLISION_OBJECT_LOADER_STATE_HPP

#include "nifloader/loader.hpp"
#include "ogrebullet/collision_shape.hpp"
#include "ogrebullet/rigid_body.hpp"

namespace oo {

/// \addtogroup OpenOBLNifloader
/// @{

void parseCollisionObject(const oo::BlockGraph &g,
                          Ogre::CollisionShape *rigidBody,
                          const nif::bhk::CollisionObject &block,
                          const Ogre::Matrix4 &transform);

using CollisionShapeVector = std::vector<Ogre::BulletCollisionShapePtr>;

std::pair<CollisionShapeVector, std::unique_ptr<Ogre::RigidBodyInfo>>
parseWorldObject(const oo::BlockGraph &g,
                 Ogre::CollisionShape *rigidBody,
                 const nif::bhk::WorldObject &block,
                 const Ogre::Matrix4 &transform);

Ogre::RigidBodyInfo generateRigidBodyInfo(const nif::bhk::RigidBody &block);

CollisionShapeVector
parseShape(const oo::BlockGraph &g,
           Ogre::CollisionShape *rigidBody,
           const nif::bhk::Shape &block,
           const Ogre::Matrix4 &transform);

CollisionShapeVector
parseShape(const oo::BlockGraph &g,
           Ogre::CollisionShape *rigidBody,
           const nif::bhk::TransformShape &block,
           const Ogre::Matrix4 &transform);

CollisionShapeVector
parseShape(const oo::BlockGraph &g,
           const nif::bhk::CapsuleShape &block,
           const Ogre::Matrix4 &transform);

CollisionShapeVector
parseShape(const oo::BlockGraph &g,
           Ogre::CollisionShape *rigidBody,
           const nif::bhk::MoppBvTreeShape &shape,
           const Ogre::Matrix4 &transform);

CollisionShapeVector
parseShape(const oo::BlockGraph &g,
           Ogre::CollisionShape *rigidBody,
           const nif::bhk::ListShape &shape,
           const Ogre::Matrix4 &transform);

CollisionShapeVector
parseShape(const oo::BlockGraph &g,
           Ogre::CollisionShape *rigidBody,
           const nif::bhk::PackedNiTriStripsShape &shape,
           const Ogre::Matrix4 &transform);

CollisionShapeVector
parseShape(const oo::BlockGraph &g,
           const nif::bhk::ConvexVerticesShape &shape,
           const Ogre::Matrix4 &transform);

CollisionShapeVector
parseShape(const oo::BlockGraph &g,
           const nif::bhk::BoxShape &shape,
           const Ogre::Matrix4 &transform);

CollisionShapeVector
parseNiTriStripsData(const oo::BlockGraph &g,
                     Ogre::CollisionShape *rigidBody,
                     const nif::hk::PackedNiTriStripsData &block,
                     const Ogre::Matrix4 &transform);

// Fill indexBuf with the indexed triangle data of block and return a pointer
// to the first byte. indexBuf will be resized if necessary.
unsigned char *fillIndexBuffer(std::vector<uint16_t> &indexBuf,
                               const nif::hk::PackedNiTriStripsData &block);

// Fill vertexBuf with the vertex data of block and return a pointer to the
// first byte. vertexBuf will be resized if necessary.
unsigned char *fillVertexBuffer(std::vector<float> &vertexBuf,
                                const nif::hk::PackedNiTriStripsData &block,
                                const Ogre::Matrix4 &transform);

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

  explicit CollisionObjectLoaderState(Ogre::CollisionShape *collisionObject,
                                      Graph blocks);

 private:
  using CollisionShapeVector = std::vector<Ogre::BulletCollisionShapePtr>;

  Ogre::CollisionShape *mRigidBody{};
  Ogre::Matrix4 mTransform{Ogre::Matrix4::IDENTITY};
  bool mHasHavok{false};
  bool mIsSkeleton{false};

  void discover_vertex(const nif::NiNode &node, const Graph &g);
  void discover_vertex(const nif::BSXFlags &bsxFlags, const Graph &g);
  void discover_vertex(const nif::BSBound &bsBound, const Graph &g);
  void discover_vertex(const nif::bhk::CollisionObject &collisionObject,
                       const Graph &g);

  void finish_vertex(const nif::NiNode &node, const Graph &g);
};

Ogre::Matrix4 getRigidBodyTransform(const nif::bhk::RigidBodyT &body);

void createCollisionObject(Ogre::CollisionShape *rigidBody,
                           oo::BlockGraph::vertex_descriptor start,
                           const oo::BlockGraph &g);

///@}

} // namespace oo

#endif // OPENOBL_NIF_COLLISION_OBJECT_LOADER_STATE_HPP
