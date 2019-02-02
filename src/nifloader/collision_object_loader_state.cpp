#include "conversions.hpp"
#include "nifloader/loader_state.hpp"
#include "nifloader/collision_object_loader_state.hpp"
#include "ogrebullet/conversions.hpp"
#include <Ogre.h>
#include <boost/graph/copy.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

namespace oo {

CollisionObjectLoaderState::CollisionObjectLoaderState(
    Ogre::CollisionObject *collisionObject,
    oo::BlockGraph blocks)
    : mRigidBody(collisionObject), mLogger(spdlog::get(oo::LOG)) {
  std::vector<boost::default_color_type> colorMap(boost::num_vertices(blocks));
  const auto propertyMap{boost::make_iterator_property_map(
      colorMap.begin(), boost::get(boost::vertex_index, blocks))};

  boost::depth_first_search(blocks, *this, propertyMap);
}

void
CollisionObjectLoaderState::start_vertex(vertex_descriptor, const Graph &) {
  mTransform = Ogre::Matrix4::IDENTITY;
}

void CollisionObjectLoaderState::discover_vertex(vertex_descriptor v,
                                                 const Graph &g) {
  const auto &block{*g[v]};

  if (dynamic_cast<const nif::NiNode *>(&block)) {
    discover_vertex(static_cast<const nif::NiNode &>(block), g);
  } else if (dynamic_cast<const nif::BSXFlags *>(&block)) {
    discover_vertex(static_cast<const nif::BSXFlags &>(block), g);
  } else if (dynamic_cast<const nif::BSBound *>(&block)) {
    discover_vertex(static_cast<const nif::BSBound &>(block), g);
  } else if (dynamic_cast<const nif::bhk::CollisionObject *>(&block)) {
    discover_vertex(static_cast<const nif::bhk::CollisionObject &>(block), g);
  }
}

void
CollisionObjectLoaderState::finish_vertex(vertex_descriptor v, const Graph &g) {
  const auto &block{*g[v]};

  if (dynamic_cast<const nif::NiNode *>(&block)) {
    finish_vertex(static_cast<const nif::NiNode &>(block), g);
  }
}

void CollisionObjectLoaderState::discover_vertex(const nif::NiNode &node,
                                                 const Graph &) {
  mTransform = mTransform * getTransform(node);
}

void CollisionObjectLoaderState::discover_vertex(const nif::BSXFlags &bsxFlags,
                                                 const Graph &) {
  using Flags = nif::BSXFlags::Flags;
  const Flags flags{bsxFlags.data};
  if ((flags & Flags::bHavok) != Flags::bNone) {
    mHasHavok = true;
  }
  if ((flags & Flags::bRagdoll) != Flags::bNone) {
    mIsSkeleton = true;
  }
}

void CollisionObjectLoaderState::discover_vertex(const nif::BSBound &bsBound,
                                                 const Graph &) {
  // btBoxShape needs to be centered at the origin, so use btConvexHullShape
  auto collisionShape{std::make_unique<btConvexHullShape>()};
  Ogre::Vector3 center{oo::fromBSCoordinates(oo::fromNif(bsBound.center))};
  Ogre::Vector3
      halfExtents{oo::fromBSCoordinates(oo::fromNif(bsBound.dimensions))};
  const Ogre::AxisAlignedBox box{halfExtents, halfExtents};

  for (const auto &corner : box.getAllCorners()) {
    collisionShape->addPoint(Ogre::toBullet(corner + center));
  }

  mRigidBody->_setCollisionShape(std::move(collisionShape));
}

void CollisionObjectLoaderState::discover_vertex(
    const nif::bhk::CollisionObject &collisionObject, const Graph &g) {
  if (!mHasHavok) return;
  parseCollisionObject(g, collisionObject);
}

void CollisionObjectLoaderState::finish_vertex(const nif::NiNode &node,
                                               const Graph &/*g*/) {
  mTransform = mTransform * getTransform(node).inverse();
}

void CollisionObjectLoaderState::parseCollisionObject(
    const Graph &g, const nif::bhk::CollisionObject &block) {
  // TODO: COFlags
  // TODO: target
  const auto &worldObj{oo::getBlock<nif::bhk::WorldObject>(g, block.body)};
  auto[collisionShapes, info]{parseWorldObject(g, worldObj)};
  if (!collisionShapes.empty()) {
    if (info && info->m_mass >= 0.1f) {
      collisionShapes.front()->setMargin(0.01f);
    }
    mRigidBody->_setCollisionShape(std::move(collisionShapes.front()));
    collisionShapes.erase(collisionShapes.begin());
    if (!collisionShapes.empty()) {
      mRigidBody->_storeIndirectCollisionShapes(std::move(collisionShapes));
    }
  }
  if (info) mRigidBody->_setRigidBodyInfo(std::move(info));
}

std::pair<CollisionObjectLoaderState::CollisionShapeVector,
          std::unique_ptr<Ogre::RigidBodyInfo>>
CollisionObjectLoaderState::parseWorldObject(
    const Graph &g, const nif::bhk::WorldObject &block) {
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
  const auto &shape{oo::getBlock<nif::bhk::Shape>(g, block.shape)};
  auto collisionShapes{parseShape(g, shape)};
  mTransform = mTransform * localTrans.inverse();

  std::unique_ptr<Ogre::RigidBodyInfo> info{};
  if (dynamic_cast<const nif::bhk::RigidBody *>(&block)
      && !collisionShapes.empty()) {
    const auto &body{dynamic_cast<const nif::bhk::RigidBody &>(block)};
    info = std::make_unique<Ogre::RigidBodyInfo>(generateRigidBodyInfo(body));
    info->m_collisionShape = collisionShapes.front().get();
  }
  return std::make_pair(std::move(collisionShapes), std::move(info));
}

Ogre::RigidBodyInfo CollisionObjectLoaderState::generateRigidBodyInfo(
    const nif::bhk::RigidBody &block) const {
  // This does not seem to affect the translation in any way.
  // TODO: What is the Havok origin used for?
  // const Ogre::Vector3 origin{oo::fromNif(block.center).xyz()};

  // Bullet needs a diagonalized inertia tensor given as a vector, and the
  // file stores it as a 3x4 matrix. We ignore the last (w) column and
  // compute the eigenvalues. Also we need to change from BS coordinates.
  const auto &hkI{block.inertiaTensor};
  const Ogre::Matrix3 bsI{hkI.m11, hkI.m12, hkI.m13,
                          hkI.m21, hkI.m22, hkI.m23,
                          hkI.m31, hkI.m32, hkI.m33};
  const Ogre::Matrix3 inertiaTensor{oo::fromBSCoordinates(bsI)};
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
  const btVector3 bulletInertia{Ogre::toBullet(principalMoments)};
  btRigidBody::btRigidBodyConstructionInfo
      info(mass, nullptr, nullptr, bulletInertia);
  info.m_linearDamping = linearDamping;
  info.m_angularDamping = angularDamping;
  info.m_friction = friction;
  info.m_restitution = restitution;

  return info;
}

//===----------------------------------------------------------------------===//
// parseShape overloads
//===----------------------------------------------------------------------===//
CollisionObjectLoaderState::CollisionShapeVector
CollisionObjectLoaderState::parseShape(const Graph &g,
                                       const nif::bhk::Shape &block) {
  using namespace nif::bhk;
  if (dynamic_cast<const TransformShape *>(&block)) {
    return parseShape(g, dynamic_cast<const TransformShape &>(block));
  } else if (dynamic_cast<const CapsuleShape *>(&block)) {
    return parseShape(g, dynamic_cast<const CapsuleShape &>(block));
  } else if (dynamic_cast<const MoppBvTreeShape *>(&block)) {
    return parseShape(g, dynamic_cast<const MoppBvTreeShape &>(block));
  } else if (dynamic_cast<const ListShape *>(&block)) {
    return parseShape(g, dynamic_cast<const ListShape &>(block));
  } else if (dynamic_cast<const PackedNiTriStripsShape *>(&block)) {
    return parseShape(g, dynamic_cast<const PackedNiTriStripsShape &>(block));
  } else if (dynamic_cast<const ConvexVerticesShape *>(&block)) {
    return parseShape(g, dynamic_cast<const ConvexVerticesShape &>(block));
  } else if (dynamic_cast<const BoxShape *>(&block)) {
    return parseShape(g, dynamic_cast<const BoxShape &>(block));
  } else {
    mLogger->warn("Parsing unknown bhkShape");
    return {};
    //OGRE_EXCEPT(Ogre::Exception::ERR_NOT_IMPLEMENTED,
    //            "Unknown collision shape",
    //            "CollisionObjectLoaderState::parseShape");
  }
}

CollisionObjectLoaderState::CollisionShapeVector
CollisionObjectLoaderState::parseShape(const Graph &g,
                                       const nif::bhk::TransformShape &block) {
  const auto &childShape{oo::getBlock<nif::bhk::Shape>(g, block.shape)};

  const Ogre::Matrix4 t{oo::fromBSCoordinates(oo::fromNif(block.transform))};
  mTransform = mTransform * t;
  auto collisionShape{parseShape(g, childShape)};
  mTransform = mTransform * t.inverse();

  return collisionShape;
}

CollisionObjectLoaderState::CollisionShapeVector
CollisionObjectLoaderState::parseShape(const Graph &,
                                       const nif::bhk::CapsuleShape &block) {
  CollisionShapeVector v;

  const Ogre::Vector3 p1{mTransform *
      oo::fromBSCoordinates(oo::fromNif(block.firstPoint)) * 7.0f};
  const Ogre::Vector3 p2{mTransform *
      oo::fromBSCoordinates(oo::fromNif(block.secondPoint)) * 7.0f};
  const float radius{oo::metersPerUnit<float> * block.radius * 7.0f};

  // Bullet capsules must be axis-aligned and the midpoint of the centres must
  // be the origin. Then, we also need to know which axis to align to.
  const Ogre::Vector3 delta{p2 - p1};
  const Ogre::Vector3 avg{(p1 + p2) / 2.0f};
  const float avgSqLen{avg.squaredLength()};
  auto isZero = [](float x) { return Ogre::Math::RealEqual(x, 0.0f, 1e-4f); };

  if (isZero(avgSqLen) && isZero(delta.y) && isZero(delta.z)) {
    v.emplace_back(std::make_unique<btCapsuleShapeX>(radius,
                                                     Ogre::Math::Abs(delta.x)));
  } else if (isZero(avgSqLen) && isZero(delta.x) && isZero(delta.z)) {
    v.emplace_back(std::make_unique<btCapsuleShape>(radius,
                                                    Ogre::Math::Abs(delta.y)));
  } else if (isZero(avgSqLen) && isZero(delta.x) && isZero(delta.y)) {
    v.emplace_back(std::make_unique<btCapsuleShapeZ>(radius,
                                                     Ogre::Math::Abs(delta.z)));
  }

  if (!v.empty()) return v;

  // Not axis-aligned, so use a btMultiSphereShape with two spheres. Bullet
  // copies the positions and radii, so passing local vectors is ok.
  std::array<btVector3, 2> positions{Ogre::toBullet(p1), Ogre::toBullet(p2)};
  std::array<btScalar, 2> radii{radius, radius};
  v.emplace_back(std::make_unique<btMultiSphereShape>(positions.data(),
                                                      radii.data(), 2));

  return v;
}

CollisionObjectLoaderState::CollisionShapeVector
CollisionObjectLoaderState::parseShape(const Graph &g,
                                       const nif::bhk::MoppBvTreeShape &shape) {
  // TODO: Use material information for collisions and sound
  //const auto material{shape.material.material};

  // Instead of decoding the MOPP data we use the linked shape
  const auto &childShape{oo::getBlock<nif::bhk::Shape>(g, shape.shape)};

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

CollisionObjectLoaderState::CollisionShapeVector
CollisionObjectLoaderState::parseShape(const Graph &g,
                                       const nif::bhk::ListShape &shape) {
  // TODO: Use material information for collisions and sound
  //const auto material{shape.material.material};

  CollisionShapeVector children;
  children.emplace_back(std::make_unique<btCompoundShape>(false));
  auto *compoundShape{static_cast<btCompoundShape *>(children.back().get())};

  for (const auto shapeRef : shape.subShapes) {
    const auto &childShape{oo::getBlock<nif::bhk::Shape>(g, shapeRef)};
    auto collisionShape{parseShape(g, childShape)};
    if (collisionShape.empty()) continue;
    compoundShape->addChildShape(btTransform::getIdentity(),
                                 collisionShape.front().get());
    children.insert(children.end(),
                    std::make_move_iterator(collisionShape.begin()),
                    std::make_move_iterator(collisionShape.end()));
  }

  return children;
}

CollisionObjectLoaderState::CollisionShapeVector
CollisionObjectLoaderState::parseShape(
    const Graph &g, const nif::bhk::PackedNiTriStripsShape &shape) {
  // TODO: Subshapes?

  const auto &data{oo::getBlock<nif::hk::PackedNiTriStripsData>(g, shape.data)};

  const Ogre::Matrix4 scaleMat = [&shape]() {
    Ogre::Matrix4 s{Ogre::Matrix4::IDENTITY};
    s.setScale(oo::fromNif(shape.scale).xyz());
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

CollisionObjectLoaderState::CollisionShapeVector
CollisionObjectLoaderState::parseShape(const Graph &/*g*/,
                                       const nif::bhk::ConvexVerticesShape &shape) {
  // TODO: Use material information for collisions and sound
  //const auto material{shape.material.material};

  auto collisionShape{std::make_unique<btConvexHullShape>()};
  for (const auto &vertex : shape.vertices) {
    const Ogre::Vector4 ogreV{oo::fromBSCoordinates(oo::fromNif(vertex).xyz()),
                              1.0f};
    const auto v{mTransform * ogreV * 7.0f};
    collisionShape->addPoint(Ogre::toBullet(v.xyz()));
  }
  CollisionShapeVector v;
  v.emplace_back(std::move(collisionShape));
  return v;
}

CollisionObjectLoaderState::CollisionShapeVector
CollisionObjectLoaderState::parseShape(const Graph &/*g*/,
                                       const nif::bhk::BoxShape &shape) {
  // TODO: Use material information for collisions and sound
  //const auto material{shape.material.material};

  // Applying the nif transform may result in a non-axis-aligned box, which
  // btBoxShape does not support, so we use a btConvexHullShape instead.
  // If necessary, one could check that the box stays axis-aligned by
  // extracting the rotation and comparing the volumes of the original
  // axis-aligned box and the rotated axis-aligned box.
  auto collisionShape{std::make_unique<btConvexHullShape>()};
  const Ogre::Vector3 halfExtents{oo::fromNif(shape.dimensions).xyz()};
  const Ogre::AxisAlignedBox box{-halfExtents, halfExtents};
  for (const auto &corner : box.getAllCorners()) {
    const Ogre::Vector4 ogreV{oo::fromBSCoordinates(corner), 1.0f};
    const auto v{mTransform * ogreV * 7.0f};
    collisionShape->addPoint(Ogre::toBullet(v.xyz()));
  }
  CollisionShapeVector v;
  v.emplace_back(std::move(collisionShape));
  return v;
}

CollisionObjectLoaderState::CollisionShapeVector
CollisionObjectLoaderState::parseNiTriStripsData(
    const Graph &/*g*/, const nif::hk::PackedNiTriStripsData &block) {
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

  mesh.m_triangleIndexBase = fillIndexBuffer(mRigidBody->_getIndexBuffer(),
                                             block);
  mesh.m_vertexBase = fillVertexBuffer(mRigidBody->_getVertexBuffer(),
                                       block);

  // Construct the actual mesh and give ownership to the rigid body.
  auto collisionMesh{std::make_unique<btTriangleIndexVertexArray>()};
  collisionMesh->addIndexedMesh(mesh, PHY_SHORT);
  auto *collisionMeshPtr{collisionMesh.get()};
  mRigidBody->_setMeshInterface(std::move(collisionMesh));

  CollisionShapeVector v;
  v.emplace_back(std::make_unique<btBvhTriangleMeshShape>(collisionMeshPtr,
                                                          false));
  return v;

  // TODO: Support dynamic concave geometry
}

//===----------------------------------------------------------------------===//
// Vertex/Index buffer functions
//===----------------------------------------------------------------------===//
unsigned char *CollisionObjectLoaderState::fillIndexBuffer(
    std::vector<uint16_t> &indexBuf,
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

unsigned char *CollisionObjectLoaderState::fillVertexBuffer(
    std::vector<float> &vertexBuf,
    const nif::hk::PackedNiTriStripsData &block) {

  vertexBuf.assign(block.numVertices * 3u, 0.0f);
  auto it{vertexBuf.begin()};
  for (const auto &vertex : block.vertices) {
    using namespace oo;
    const Ogre::Vector4 ogreV{fromBSCoordinates(fromNif(vertex))};
    const auto v{mTransform * ogreV};
    *it++ = v.x;
    *it++ = v.y;
    *it++ = v.z;
  }
  return reinterpret_cast<unsigned char *>(vertexBuf.data());
}

Ogre::Matrix4 getRigidBodyTransform(const nif::bhk::RigidBodyT &body) {
  using namespace oo;
  Ogre::Matrix4 t{Ogre::Matrix4::IDENTITY};
  t.makeTransform(fromBSCoordinates(fromNif(body.translation).xyz()),
                  Ogre::Vector3::UNIT_SCALE,
                  fromBSCoordinates(fromNif(body.rotation)));
  return t;
}

} // namespace oo
