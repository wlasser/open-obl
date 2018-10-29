#include "engine/conversions.hpp"
#include "engine/nifloader/loader_state.hpp"
#include "engine/nifloader/collision_object_loader_state.hpp"
#include "ogrebullet/conversions.hpp"
#include <Ogre.h>
#include <boost/graph/copy.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

namespace engine::nifloader {

CollisionObjectLoaderState::CollisionObjectLoaderState(
    Ogre::CollisionObject *collisionObject,
    nifloader::BlockGraph blocks) {

  std::vector<boost::default_color_type> colorMap(boost::num_vertices(blocks));
  const auto indexMap{boost::get(boost::vertex_index, blocks)};
  const auto propertyMap{boost::make_iterator_property_map(colorMap.begin(),
                                                           indexMap)};
  CollisionObjectVisitor visitor(collisionObject);
  boost::depth_first_search(blocks, std::move(visitor), propertyMap);
}

void CollisionObjectVisitor::start_vertex(vertex_descriptor, const Graph &) {
  mTransform = Ogre::Matrix4::IDENTITY;
}

void CollisionObjectVisitor::discover_vertex(vertex_descriptor v,
                                             const Graph &g) {
  using namespace nif;
  const auto &niObject{*g[v]};

  if (dynamic_cast<const NiNode *>(&niObject)) {
    discover_vertex(dynamic_cast<const NiNode &>(niObject), g);
  } else if (dynamic_cast<const BSXFlags *>(&niObject)) {
    discover_vertex(dynamic_cast<const BSXFlags &>(niObject), g);
  } else if (dynamic_cast<const bhk::CollisionObject *>(&niObject)) {
    discover_vertex(dynamic_cast<const bhk::CollisionObject &>(niObject), g);
  }
}

void CollisionObjectVisitor::finish_vertex(vertex_descriptor v,
                                           const Graph &g) {
  using namespace nif;
  const auto &niObject{*g[v]};

  if (dynamic_cast<const NiNode *>(&niObject)) {
    finish_vertex(dynamic_cast<const NiNode &>(niObject), g);
  }
}

void CollisionObjectVisitor::discover_vertex(const nif::NiNode &node,
                                             const Graph &) {
  mTransform = mTransform * getTransform(node);
}

void CollisionObjectVisitor::discover_vertex(const nif::BSXFlags &bsxFlags,
                                             const Graph &) {
  using Flags = nif::BSXFlags::Flags;
  const Flags flags{bsxFlags.data};
  if ((flags & Flags::bHavok) != Flags::bNone) {
    mHasHavok = true;
  }
}

void CollisionObjectVisitor::discover_vertex(const nif::bhk::CollisionObject &collisionObject,
                                             const Graph &g) {
  if (!mHasHavok) return;
  parseCollisionObject(g, collisionObject);
}

void CollisionObjectVisitor::finish_vertex(const nif::NiNode &node,
                                           const Graph &g) {
  mTransform = mTransform * getTransform(node).inverse();
}

void CollisionObjectVisitor::parseCollisionObject(const Graph &g,
                                                  const nif::bhk::CollisionObject &block) {
  // TODO: COFlags
  // TODO: target
  const auto &worldObj{getRef<nif::bhk::WorldObject>(g, block.body)};
  auto[collisionShape, info]{parseWorldObject(g, worldObj)};
  if (collisionShape) mRigidBody->mCollisionShape = std::move(collisionShape);
  if (info) mRigidBody->mInfo = std::move(info);
}

std::pair<std::unique_ptr<btCollisionShape>,
          std::unique_ptr<Ogre::RigidBodyInfo>>
CollisionObjectVisitor::parseWorldObject(const Graph &g,
                                         const nif::bhk::WorldObject &block) {
  // TODO: Flags

  const Ogre::Matrix4 localTrans = [&block]() {
    // TODO: RigidBody that is not a RigidBodyT
    if (dynamic_cast<const nif::bhk::RigidBodyT *>(&block)) {
      const auto &body{dynamic_cast<const nif::bhk::RigidBodyT &>(block)};
      return getRigidBodyTransform(body);
    } else {
      return Ogre::Matrix4::IDENTITY;
    }
  }();
  mTransform = mTransform * localTrans;
  const auto &shape{getRef<nif::bhk::Shape>(g, block.shape)};
  auto collisionShape{parseShape(g, shape)};
  mTransform = mTransform * localTrans.inverse();

  std::unique_ptr<Ogre::RigidBodyInfo> info{};
  if (dynamic_cast<const nif::bhk::RigidBody *>(&block)) {
    const auto &body{dynamic_cast<const nif::bhk::RigidBody &>(block)};
    info = std::make_unique<Ogre::RigidBodyInfo>(generateRigidBodyInfo(body));
    info->m_collisionShape = collisionShape.get();
  }
  return std::make_pair(std::move(collisionShape), std::move(info));
}

Ogre::RigidBodyInfo
CollisionObjectVisitor::generateRigidBodyInfo(const nif::bhk::RigidBody &block) const {
  using namespace engine::conversions;
  using namespace Ogre::conversions;

  // This does not seem to affect the translation in any way.
  // TODO: What is the Havok origin used for?
  const Ogre::Vector3 origin{fromNif(block.center).xyz()};

  // Bullet needs a diagonalized inertia tensor given as a vector, and the
  // file stores it as a 3x4 matrix. We ignore the last (w) column and
  // compute the eigenvalues. Also we need to change from BS coordinates.
  const auto &hkI{block.inertiaTensor};
  const Ogre::Matrix3 bsI{hkI.m11, hkI.m12, hkI.m13,
                          hkI.m21, hkI.m22, hkI.m23,
                          hkI.m31, hkI.m32, hkI.m33};
  const Ogre::Matrix3 inertiaTensor{fromBSCoordinates(bsI)};
  std::array<Ogre::Vector3, 3> principalAxes{};
  Ogre::Vector3 principalMoments{};
  inertiaTensor.EigenSolveSymmetric(principalMoments.ptr(),
                                    principalAxes.data());
  // Coordinates in havok are scaled up by a factor of 7, and the units of the
  // moments of inertia are kg m^2.
  principalMoments /= (7.0f * 7.0f);

  // We have the diagonalization
  // inertiaTensor = principalAxes * diag(principalMoments) * principalAxes^T
  // To do this properly we should change to the principal frame but for now
  // assume the principal axes are coordinate-axis aligned (this seems to hold
  // in practice).
  // TODO: Change to principal frame

  // The units for these are the same in Bullet and Havok
  const float mass{block.mass};
  const float linearDamping{block.linearDamping};
  const float angularDamping{block.angularDamping};
  const float friction{block.friction};
  const float restitution{block.restitution};

  // TODO: Constraints

  // Convert the Ogre parameters to Bullet ones and set up the rigid body
  const btVector3 bulletInertia{toBullet(principalMoments)};
  btRigidBody::btRigidBodyConstructionInfo
      info(mass, nullptr, nullptr, bulletInertia);
  info.m_linearDamping = linearDamping;
  info.m_angularDamping = angularDamping;
  info.m_friction = friction;
  info.m_restitution = restitution;

  return info;
}

std::unique_ptr<btCollisionShape>
CollisionObjectVisitor::parseShape(const Graph &g,
                                   const nif::bhk::Shape &block) {
  using namespace nif::bhk;
  if (dynamic_cast<const MoppBvTreeShape *>(&block)) {
    return parseShape(g, dynamic_cast<const MoppBvTreeShape &>(block));
  } else if (dynamic_cast<const PackedNiTriStripsShape *>(&block)) {
    return parseShape(g, dynamic_cast<const PackedNiTriStripsShape &>(block));
  } else if (dynamic_cast<const ConvexVerticesShape *>(&block)) {
    return parseShape(g, dynamic_cast<const ConvexVerticesShape &>(block));
  } else if (dynamic_cast<const BoxShape *>(&block)) {
    return parseShape(g, dynamic_cast<const BoxShape &>(block));
  } else {
    mLogger->warn("Parsing unknown bhkShape");
    return nullptr;
    //OGRE_EXCEPT(Ogre::Exception::ERR_NOT_IMPLEMENTED,
    //            "Unknown collision shape",
    //            "CollisionObjectVisitor::parseShape");
  }
}

std::unique_ptr<btCollisionShape>
CollisionObjectVisitor::parseShape(const Graph &g,
                                   const nif::bhk::MoppBvTreeShape &shape) {
  const auto material{shape.material.material};

  // Instead of decoding the MOPP data we use the linked shape
  const auto &childShape{getRef<nif::bhk::Shape>(g, shape.shape)};

  // Apply the scale and recurse into the linked shape
  // The effort here is to avoid scaling the w component
  const float scale{shape.shapeScale};
  const Ogre::Matrix4 scaleMat = [scale]() {
    Ogre::Matrix4 s{Ogre::Matrix4::IDENTITY};
    s.setScale({scale, scale, scale});
    return s;
  }();

  mTransform = mTransform * scaleMat;
  auto collisionShape{parseShape(g, childShape)};
  mTransform = mTransform * scaleMat.inverse();

  return collisionShape;
}

std::unique_ptr<btCollisionShape>
CollisionObjectVisitor::parseShape(const Graph &g,
                                   const nif::bhk::PackedNiTriStripsShape &shape) {
  // TODO: Subshapes?

  const auto &data{getRef<nif::hk::PackedNiTriStripsData>(g, shape.data)};

  const Ogre::Matrix4 scaleMat = [&shape]() {
    Ogre::Matrix4 s{Ogre::Matrix4::IDENTITY};
    s.setScale(conversions::fromNif(shape.scale).xyz());
    return s;
  }();

  // For some reason the coordinates are scaled down in the nif file by a
  // factor of 7. This scale needs to also apply to the translation, not
  // just the linear part.
  mTransform = mTransform * scaleMat * 7.0f;
  auto collisionShape{parseNiTriStripsData(g, data)};
  mTransform = mTransform * scaleMat.inverse() * (1.0f / 7.0f);

  return collisionShape;
}

std::unique_ptr<btCollisionShape>
CollisionObjectVisitor::parseShape(const Graph &g,
                                   const nif::bhk::ConvexVerticesShape &shape) {
  const auto material{shape.material.material};

  auto collisionShape{std::make_unique<btConvexHullShape>()};
  for (const auto &vertex : shape.vertices) {
    using namespace engine::conversions;
    using namespace Ogre::conversions;
    const Ogre::Vector4 ogreV{fromBSCoordinates(fromNif(vertex).xyz()), 1.0f};
    const auto v{mTransform * ogreV * 7.0f};
    collisionShape->addPoint(toBullet(v.xyz()));
  }
  return collisionShape;
}

std::unique_ptr<btCollisionShape>
CollisionObjectVisitor::parseShape(const Graph &g,
                                   const nif::bhk::BoxShape &shape) {
  const auto material{shape.material.material};

  // Applying the nif transform may result in a non-axis-aligned box, which
  // btBoxShape does not support, so we use a btConvexHullShape instead.
  // If necessary, one could check that the box stays axis-aligned by
  // extracting the rotation and comparing the volumes of the original
  // axis-aligned box and the rotated axis-aligned box.
  using namespace engine::conversions;
  using namespace Ogre::conversions;
  auto collisionShape{std::make_unique<btConvexHullShape>()};
  const Ogre::Vector3 halfExtents{fromNif(shape.dimensions).xyz()};
  const Ogre::AxisAlignedBox box{halfExtents, halfExtents};
  for (const auto &corner : box.getAllCorners()) {
    const Ogre::Vector4 ogreV{fromBSCoordinates(corner), 1.0f};
    const auto v{mTransform * ogreV * 7.0f};
    collisionShape->addPoint(toBullet(v.xyz()));
  }
  return collisionShape;
}

std::unique_ptr<btCollisionShape>
CollisionObjectVisitor::parseNiTriStripsData(const Graph &g,
                                             const nif::hk::PackedNiTriStripsData &block) {
  // For static geometry we construct a btBvhTriangleMeshShape using indexed
  // triangles. Bullet doesn't copy the underlying vertex and index buffers,
  // so they need to be kept alive for the lifetime of the collision object.
  btIndexedMesh mesh{};

  mesh.m_numTriangles = block.numTriangles;
  mesh.m_numVertices = block.numVertices;

  // TODO: Is aligning the triangles to 8-byte boundaries faster than packing?
  mesh.m_triangleIndexStride = 3u * sizeof(nif::basic::UShort);

  // Vertex data is always in single-precision, regardless of Ogre or Bullet
  mesh.m_vertexType = PHY_FLOAT;
  mesh.m_vertexStride = 3u * sizeof(float);

  mesh.m_triangleIndexBase = fillIndexBuffer(mRigidBody->mIndexBuffer, block);
  mesh.m_vertexBase = fillVertexBuffer(mRigidBody->mVertexBuffer, block);

  // Construct the actual mesh and give ownership to the rigid body
  auto collisionMesh{std::make_unique<btTriangleIndexVertexArray>()};
  collisionMesh->addIndexedMesh(mesh, PHY_SHORT);
  mRigidBody->mCollisionMesh = std::move(collisionMesh);

  return std::make_unique<btBvhTriangleMeshShape>(
      mRigidBody->mCollisionMesh.get(), false);

  // TODO: Support dynamic concave geometry
}

unsigned char *
CollisionObjectVisitor::fillIndexBuffer(std::vector<uint16_t> &indexBuf,
                                        const nif::hk::PackedNiTriStripsData &block) {
  indexBuf.assign(block.numTriangles * 3u, 0u);
  auto it{indexBuf.begin()};
  for (const auto &triData : block.triangles) {
    const auto &tri{triData.triangle};
    *it++ = tri.v1;
    *it++ = tri.v2;
    *it++ = tri.v3;
  }
  return reinterpret_cast<unsigned char *>(indexBuf.data());
}

unsigned char *
CollisionObjectVisitor::fillVertexBuffer(std::vector<float> &vertexBuf,
                                         const nif::hk::PackedNiTriStripsData &block) {
  vertexBuf.assign(block.numVertices * 3u, 0.0f);
  auto it{vertexBuf.begin()};
  for (const auto &vertex : block.vertices) {
    using namespace engine::conversions;
    const Ogre::Vector4 ogreV{fromBSCoordinates(fromNif(vertex))};
    const auto v{mTransform * ogreV};
    *it++ = v.x;
    *it++ = v.y;
    *it++ = v.z;
  }
  return reinterpret_cast<unsigned char *>(vertexBuf.data());
}

Ogre::Matrix4 getRigidBodyTransform(const nif::bhk::RigidBodyT &body) {
  using namespace engine::conversions;
  Ogre::Matrix4 t{Ogre::Matrix4::IDENTITY};
  t.makeTransform(fromBSCoordinates(fromNif(body.translation).xyz()),
                  Ogre::Vector3::UNIT_SCALE,
                  fromBSCoordinates(fromNif(body.rotation)));
  return t;
}

} // namespace engine::nifloader
