#include "bullet/collision.hpp"
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

void createCollisionObject(Ogre::CollisionShape *rigidBody,
                           oo::BlockGraph::vertex_descriptor start,
                           const oo::BlockGraph &g) {
  const auto &rootBlock{*g[start]};
  if (!dynamic_cast<const nif::NiNode *>(&rootBlock)) {
    OGRE_EXCEPT(Ogre::Exception::ERR_INVALIDPARAMS,
                "Cannot create a CollisionShape with a root node that is not "
                "an NiNode",
                "oo::createCollisionObject");
  }

  spdlog::get(oo::LOG)->info("createCollisionObject({})", rigidBody->getName());

  // It is assumed that the transformation of the root node will be applied via
  // its Ogre::Node transformation, and thus we do not need to bake it in.
  const Ogre::Matrix4 transform{Ogre::Matrix4::IDENTITY};

  for (const auto &e : boost::make_iterator_range(boost::out_edges(start, g))) {
    auto v{boost::target(e, g)};
    const auto &block{*g[v]};
    if (!dynamic_cast<const nif::bhk::CollisionObject *>(&block)) continue;

    const auto &obj{static_cast<const nif::bhk::CollisionObject &>(block)};
    oo::parseCollisionObject(g, rigidBody, obj, transform);
  }
}

void parseCollisionObject(const oo::BlockGraph &g,
                          Ogre::CollisionShape *rigidBody,
                          const nif::bhk::CollisionObject &block,
                          const Ogre::Matrix4 &transform) {
  // TODO: COFlags
  // TODO: target
  const auto &worldObj{oo::getBlock<nif::bhk::WorldObject>(g, block.body)};
  auto[shapes, info]{oo::parseWorldObject(g, rigidBody, worldObj, transform)};

  if (!shapes.empty()) {
    if (info && info->m_mass >= 0.1f) shapes.front()->setMargin(0.01f);
    rigidBody->_setCollisionShape(std::move(shapes.front()));
    if (shapes.erase(shapes.begin()) != shapes.end()) {
      rigidBody->_storeIndirectCollisionShapes(std::move(shapes));
    }
  }

  if (info) rigidBody->_setRigidBodyInfo(std::move(info));
}

std::pair<oo::CollisionShapeVector, std::unique_ptr<Ogre::RigidBodyInfo>>
parseWorldObject(const oo::BlockGraph &g,
                 Ogre::CollisionShape *rigidBody,
                 const nif::bhk::WorldObject &block,
                 const Ogre::Matrix4 &transform) {
  // TODO: Flags
  if (dynamic_cast<const nif::bhk::RigidBody *>(&block)) {
    const auto &body{dynamic_cast<const nif::bhk::RigidBody &>(block)};

    // MotionType does not seem to have an analogue in Bullet, but we can try to
    // match the (perceived) intent using CollisionFlags. In Bullet, a
    // `btRigidBody` can be static, kinematic, or dynamic; there is no invalid.
    // It seems reasonable for keyframed to correspond to kinematic, and fixed
    // to correspond to static; I'm unsure of the specifics of the other
    // flags, but they seem to be used for movable objects so default to
    // dynamic.
    using MotionType = nif::Enum::hk::MotionType;
    switch (body.motionSystem) {
      default: [[fallthrough]];
      case MotionType::MO_SYS_INVALID: [[fallthrough]];
      case MotionType::MO_SYS_DYNAMIC: [[fallthrough]];
      case MotionType::MO_SYS_SPHERE_INERTIA: [[fallthrough]];
      case MotionType::MO_SYS_SPHERE_STABILIZED: [[fallthrough]];
      case MotionType::MO_SYS_BOX_INERTIA: [[fallthrough]];
      case MotionType::MO_SYS_BOX_STABILIZED: [[fallthrough]];
      case MotionType::MO_SYS_THIN_BOX: [[fallthrough]];
      case MotionType::MO_SYS_CHARACTER: {
        rigidBody->setCollisionObjectType(Ogre::CollisionShape::COT_DYNAMIC);
        break;
      }
      case MotionType::MO_SYS_KEYFRAMED: {
        rigidBody->setCollisionObjectType(Ogre::CollisionShape::COT_KINEMATIC);
        break;
      }
      case MotionType::MO_SYS_FIXED: {
        rigidBody->setCollisionObjectType(Ogre::CollisionShape::COT_STATIC);
        break;
      }
    }

    // Bullet does not have different deactivators, but we can simulate
    // DEACTIVATOR_NEVER by disabling deactivation on the given collision shape.
    // On the other hand, static objects usually have DEACTIVATOR_NEVER. I can't
    // think of a reason why they should not be deactivated when inactive, since
    // they never move, so we optimize away and allow static objects to
    // deactivate regardless of the flag.
    using DeactivatorType = nif::Enum::hk::DeactivatorType;
    switch (body.deactivatorType) {
      default: [[fallthrough]];
      case DeactivatorType::DEACTIVATOR_INVALID: [[fallthrough]];
      case DeactivatorType::DEACTIVATOR_SPATIAL: {
        rigidBody->setAllowDeactivationEnabled(true);
        break;
      }
      case DeactivatorType::DEACTIVATOR_NEVER: {
        rigidBody->setAllowDeactivationEnabled(
            rigidBody->getCollisionObjectType()
                == Ogre::CollisionShape::COT_STATIC);
        break;
      }
    }
  }

  bullet::setCollisionLayer(gsl::make_not_null(rigidBody),
                            block.havokFilter.layer);

  const Ogre::Matrix4 localTrans = [&block, &transform]() {
    // TODO: RigidBody that is not a RigidBodyT
    if (dynamic_cast<const nif::bhk::RigidBodyT *>(&block)) {
      const auto &body{dynamic_cast<const nif::bhk::RigidBodyT &>(block)};
      return transform * oo::getRigidBodyTransform(body);
    } else {
      return transform;
    }
  }();
  const auto &shape{oo::getBlock<nif::bhk::Shape>(g, block.shape)};
  auto shapes{oo::parseShape(g, rigidBody, shape, localTrans)};

  std::unique_ptr<Ogre::RigidBodyInfo> info{};
  if (dynamic_cast<const nif::bhk::RigidBody *>(&block) && !shapes.empty()) {
    const auto &body{dynamic_cast<const nif::bhk::RigidBody &>(block)};
    info = std::make_unique<Ogre::RigidBodyInfo>(
        oo::generateRigidBodyInfo(body));
    info->m_collisionShape = shapes.front().get();
  }
  return std::make_pair(std::move(shapes), std::move(info));
}

Ogre::RigidBodyInfo generateRigidBodyInfo(const nif::bhk::RigidBody &block) {
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
  principalMoments *= Ogre::Math::Sqr(oo::havokUnitsPerUnit<Ogre::Real>);

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
  const auto bulletInertia{qvm::convert_to<btVector3>(principalMoments)};
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

oo::CollisionShapeVector
parseShape(const oo::BlockGraph &g, Ogre::CollisionShape *rigidBody,
           const nif::bhk::Shape &block, const Ogre::Matrix4 &transform) {
  if (dynamic_cast<const nif::bhk::TransformShape *>(&block)) {
    const auto &b{static_cast<const nif::bhk::TransformShape &>(block)};
    return oo::parseShape(g, rigidBody, b, transform);
  } else if (dynamic_cast<const nif::bhk::CapsuleShape *>(&block)) {
    const auto &b{static_cast<const nif::bhk::CapsuleShape &>(block)};
    return oo::parseShape(g, b, transform);
  } else if (dynamic_cast<const nif::bhk::MoppBvTreeShape *>(&block)) {
    const auto &b{static_cast<const nif::bhk::MoppBvTreeShape &>(block)};
    return oo::parseShape(g, rigidBody, b, transform);
  } else if (dynamic_cast<const nif::bhk::ListShape *>(&block)) {
    const auto &b{static_cast<const nif::bhk::ListShape &>(block)};
    return oo::parseShape(g, rigidBody, b, transform);
  } else if (dynamic_cast<const nif::bhk::PackedNiTriStripsShape *>(&block)) {
    const auto &b{static_cast<const nif::bhk::PackedNiTriStripsShape &>(block)};
    return oo::parseShape(g, rigidBody, b, transform);
  } else if (dynamic_cast<const nif::bhk::ConvexVerticesShape *>(&block)) {
    const auto &b{static_cast<const nif::bhk::ConvexVerticesShape &>(block)};
    return oo::parseShape(g, b, transform);
  } else if (dynamic_cast<const nif::bhk::BoxShape *>(&block)) {
    const auto &b{static_cast<const nif::bhk::BoxShape &>(block)};
    return oo::parseShape(g, b, transform);
  } else {
    spdlog::get(oo::LOG)->warn("Parsing unknown bhkShape");
    return {};
    //OGRE_EXCEPT(Ogre::Exception::ERR_NOT_IMPLEMENTED,
    //            "Unknown collision shape",
    //            "oo::parseShape");
  }
}

oo::CollisionShapeVector
parseShape(const oo::BlockGraph &g,
           Ogre::CollisionShape *rigidBody,
           const nif::bhk::TransformShape &block,
           const Ogre::Matrix4 &transform) {
  const auto &childShape{oo::getBlock<nif::bhk::Shape>(g, block.shape)};
  const Ogre::Matrix4 t{oo::fromBSCoordinates(block.transform)};
  return oo::parseShape(g, rigidBody, childShape, transform * t);
}

oo::CollisionShapeVector
parseShape(const oo::BlockGraph &,
           const nif::bhk::CapsuleShape &block,
           const Ogre::Matrix4 &transform) {
  CollisionShapeVector v;

  const Ogre::Vector3 p1
      {transform * oo::fromHavokCoordinates(block.firstPoint)};
  const Ogre::Vector3 p2
      {transform * oo::fromHavokCoordinates(block.secondPoint)};
  const float radius
      {oo::metersPerUnit<float> * oo::unitsPerHavokUnit<float> * block.radius};

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
  std::array<btVector3, 2> positions{
      qvm::convert_to<btVector3>(p1), qvm::convert_to<btVector3>(p2)
  };
  std::array<btScalar, 2> radii{radius, radius};
  v.emplace_back(std::make_unique<btMultiSphereShape>(positions.data(),
                                                      radii.data(), 2));

  return v;
}

oo::CollisionShapeVector
parseShape(const oo::BlockGraph &g,
           Ogre::CollisionShape *rigidBody,
           const nif::bhk::MoppBvTreeShape &shape,
           const Ogre::Matrix4 &transform) {
  // TODO: Use material information for collisions and sound
  //const auto material{shape.material.material};

  // Instead of decoding the MOPP data we use the linked shape
  const auto &childShape{oo::getBlock<nif::bhk::Shape>(g, shape.shape)};

  // Apply the scale and recurse into the linked shape
  using namespace qvm::sfinae;
  const auto &scaleMat{qvm::diag_mat(qvm::XXX1(shape.shapeScale))};

  return oo::parseShape(g, rigidBody, childShape, transform * scaleMat);
}

oo::CollisionShapeVector
parseShape(const oo::BlockGraph &g,
           Ogre::CollisionShape *rigidBody,
           const nif::bhk::ListShape &shape,
           const Ogre::Matrix4 &transform) {
  // TODO: Use material information for collisions and sound
  //const auto material{shape.material.material};

  oo::CollisionShapeVector children;
  children.emplace_back(std::make_unique<btCompoundShape>(false));
  auto *compoundShape{static_cast<btCompoundShape *>(children.back().get())};

  for (const auto shapeRef : shape.subShapes) {
    const auto &childShape{oo::getBlock<nif::bhk::Shape>(g, shapeRef)};
    auto collisionShape{oo::parseShape(g, rigidBody, childShape, transform)};
    if (collisionShape.empty()) continue;
    compoundShape->addChildShape(btTransform::getIdentity(),
                                 collisionShape.front().get());
    children.insert(children.end(),
                    std::make_move_iterator(collisionShape.begin()),
                    std::make_move_iterator(collisionShape.end()));
  }

  return children;
}

oo::CollisionShapeVector
parseShape(const oo::BlockGraph &g,
           Ogre::CollisionShape *rigidBody,
           const nif::bhk::PackedNiTriStripsShape &shape,
           const Ogre::Matrix4 &transform) {
  // TODO: Subshapes?

  const auto &data{oo::getBlock<nif::hk::PackedNiTriStripsData>(g, shape.data)};

  using namespace qvm::sfinae;
  const auto &havokScaleMat{qvm::diag_mat(qvm::XYZ1(shape.scale))};
  const Ogre::Matrix4 scaleMat{oo::fromHavokCoordinates(havokScaleMat)};

  return oo::parseNiTriStripsData(g, rigidBody, data, transform * scaleMat);
}

CollisionShapeVector
parseShape(const oo::BlockGraph &, const nif::bhk::ConvexVerticesShape &shape,
           const Ogre::Matrix4 &transform) {
  // TODO: Use material information for collisions and sound
  //const auto material{shape.material.material};

  auto collisionShape{std::make_unique<btConvexHullShape>()};
  for (const auto &vertex : shape.vertices) {
    const auto v{transform * oo::fromHavokCoordinates(vertex)};
    collisionShape->addPoint(qvm::convert_to<btVector3>(qvm::XYZ(v)));
  }
  CollisionShapeVector v;
  v.emplace_back(std::move(collisionShape));
  return v;
}

CollisionShapeVector
parseShape(const oo::BlockGraph &, const nif::bhk::BoxShape &shape,
           const Ogre::Matrix4 &transform) {
  // TODO: Use material information for collisions and sound
  //const auto material{shape.material.material};

  // Applying the nif transform may result in a non-axis-aligned box, which
  // btBoxShape does not support, so we use a btConvexHullShape instead.
  // If necessary, one could check that the box stays axis-aligned by
  // extracting the rotation and comparing the volumes of the original
  // axis-aligned box and the rotated axis-aligned box.
  auto collisionShape{std::make_unique<btConvexHullShape>()};
  const auto &qvmHalfExtents{qvm::XYZ(shape.dimensions)};
  const auto halfExtents{qvm::convert_to<Ogre::Vector3>(qvmHalfExtents)};
  const Ogre::AxisAlignedBox box{-halfExtents, halfExtents};
  for (const auto &corner : box.getAllCorners()) {
    using namespace qvm::sfinae;
    const auto v{transform * qvm::XYZ1(oo::fromHavokCoordinates(corner))};
    collisionShape->addPoint(qvm::convert_to<btVector3>(qvm::XYZ(v)));
  }
  CollisionShapeVector v;
  v.emplace_back(std::move(collisionShape));
  return v;
}

CollisionShapeVector
parseNiTriStripsData(const oo::BlockGraph &, Ogre::CollisionShape *rigidBody,
                     const nif::hk::PackedNiTriStripsData &block,
                     const Ogre::Matrix4 &transform) {
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

  mesh.m_triangleIndexBase = oo::fillIndexBuffer(rigidBody->_getIndexBuffer(),
                                                 block);
  mesh.m_vertexBase = oo::fillVertexBuffer(rigidBody->_getVertexBuffer(),
                                           block,
                                           transform);

  // Construct the actual mesh and give ownership to the rigid body.
  auto collisionMesh{std::make_unique<btTriangleIndexVertexArray>()};
  collisionMesh->addIndexedMesh(mesh, PHY_SHORT);
  auto *collisionMeshPtr{collisionMesh.get()};
  rigidBody->_setMeshInterface(std::move(collisionMesh));

  CollisionShapeVector v;
  v.emplace_back(std::make_unique<btBvhTriangleMeshShape>(collisionMeshPtr,
                                                          false));
  return v;

  // TODO: Support dynamic concave geometry
}

//===----------------------------------------------------------------------===//
// Vertex/Index buffer functions
//===----------------------------------------------------------------------===//
unsigned char *fillIndexBuffer(std::vector<uint16_t> &indexBuf,
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

unsigned char *fillVertexBuffer(std::vector<float> &vertexBuf,
                                const nif::hk::PackedNiTriStripsData &block,
                                const Ogre::Matrix4 &transform) {
  vertexBuf.assign(block.numVertices * 3u, 0.0f);
  auto it{vertexBuf.begin()};
  for (const auto &vertex : block.vertices) {
    using namespace oo;
    const auto v{transform * oo::fromHavokCoordinates(vertex)};
    *it++ = v.x;
    *it++ = v.y;
    *it++ = v.z;
  }
  return reinterpret_cast<unsigned char *>(vertexBuf.data());
}

Ogre::Matrix4 getRigidBodyTransform(const nif::bhk::RigidBodyT &body) {
  Ogre::Matrix4 t{Ogre::Matrix4::IDENTITY};
  t.makeTransform(oo::fromHavokCoordinates(qvm::XYZ(body.translation)),
                  Ogre::Vector3::UNIT_SCALE,
                  oo::fromHavokCoordinates(body.rotation));
  return t;
}

//===----------------------------------------------------------------------===//
// CollisionObjectLoaderState definitions
//===----------------------------------------------------------------------===//

CollisionObjectLoaderState::CollisionObjectLoaderState(
    Ogre::CollisionShape *collisionObject, oo::BlockGraph blocks)
    : mRigidBody(collisionObject) {
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
  Ogre::Vector3 center{oo::fromBSCoordinates(bsBound.center)};
  Ogre::Vector3 halfExtents{oo::fromBSCoordinates(bsBound.dimensions)};
  const Ogre::AxisAlignedBox box{halfExtents, halfExtents};

  for (const auto &corner : box.getAllCorners()) {
    collisionShape->addPoint(qvm::convert_to<btVector3>(corner + center));
  }

  mRigidBody->_setCollisionShape(std::move(collisionShape));
}

void CollisionObjectLoaderState::discover_vertex(
    const nif::bhk::CollisionObject &collisionObject, const Graph &g) {
  if (!mHasHavok) return;
  oo::parseCollisionObject(g, mRigidBody, collisionObject, mTransform);
}

void CollisionObjectLoaderState::finish_vertex(const nif::NiNode &node,
                                               const Graph &/*g*/) {
  mTransform = mTransform * getTransform(node).inverse();
}

} // namespace oo
