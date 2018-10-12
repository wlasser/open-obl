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
    nifloader::BlockGraph untaggedBlocks) {

  CollisionObjectVisitor::Graph blocks{};
  boost::copy_graph(untaggedBlocks, blocks);

  std::vector<boost::default_color_type> colorMap(boost::num_vertices(blocks));
  auto propertyMap = boost::make_iterator_property_map(
      colorMap.begin(), boost::get(boost::vertex_index, blocks));

  boost::depth_first_search(blocks,
                            CollisionObjectVisitor(collisionObject),
                            propertyMap);
}

void CollisionObjectVisitor::start_vertex(vertex_descriptor v,
                                          const Graph &g) {
  mTransform = Ogre::Matrix4::IDENTITY;
  mLogger->trace("Started at root vertex {}, reset transform", v);
}

void CollisionObjectVisitor::discover_vertex(vertex_descriptor v,
                                             const Graph &g) {
  auto &taggedNiObject = g[v];
  auto *niObject = taggedNiObject.block.get();
  auto &tag = taggedNiObject.tag;

  if (auto niNode = dynamic_cast<nif::NiNode *>(niObject)) {
    engine::nifloader::Tagger tagger{tag};
    mLogger->trace("Parsing block {} (NiNode)", v);
    mTransform = mTransform * engine::nifloader::getTransform(niNode);
    mLogger->trace(" * New transform = {}", mTransform);
  } else if (auto bsxFlags = dynamic_cast<nif::BSXFlags *>(niObject)) {
    engine::nifloader::Tagger tagger{tag};
    mLogger->trace("Parsing block {} (BSXFlags)", v);
    mLogger->trace(" * New transform = {}", mTransform);
    using Flags = nif::BSXFlags::Flags;
    auto flags = Flags(bsxFlags->data);
    if ((flags & Flags::bHavok) != Flags::bNone) {
      mHasHavok = true;
    }
  } else if (auto collisionObject =
      dynamic_cast<nif::bhk::CollisionObject *>(niObject)) {
    if (!mHasHavok) return;
    parseCollisionObject(g, collisionObject, tag);
  }
}

void CollisionObjectVisitor::finish_vertex(vertex_descriptor v,
                                           const Graph &g) {
  auto *niObject = g[v].block.get();
  if (auto niNode = dynamic_cast<nif::NiNode *>(niObject)) {
    mTransform = mTransform * engine::nifloader::getTransform(niNode).inverse();
  }
  mLogger->trace("Finished block {}", v);
  mLogger->trace(" * New transform = {}", mTransform);
}

void CollisionObjectVisitor::parseCollisionObject(const Graph &g,
                                                  nif::bhk::CollisionObject *block,
                                                  engine::nifloader::LoadStatus &tag) {
  engine::nifloader::Tagger tagger{tag};
  mLogger->trace("Parsing block ? (bhkCollisionObject)");
  mLogger->trace(" * New transform = {}", mTransform);

  // TODO: COFlags
  // TODO: target
  auto[worldObj, worldObjTag] = getRef<nif::bhk::WorldObject>(g, block->body);
  auto[collisionShape, info] = parseWorldObject(g, worldObj, worldObjTag);
  if (collisionShape) mRigidBody->mCollisionShape = std::move(collisionShape);
  if (info) mRigidBody->mInfo = std::move(info);
}

std::pair<std::unique_ptr<btCollisionShape>,
          std::unique_ptr<Ogre::RigidBodyInfo>>
CollisionObjectVisitor::parseWorldObject(const Graph &g,
                                         nif::bhk::WorldObject *block,
                                         engine::nifloader::LoadStatus &tag) {
  engine::nifloader::Tagger tagger{tag};
  mLogger->trace("Parsing block ? (bhkWorldObject)");
  mLogger->trace(" * New transform = {}", mTransform);

  // TODO: Flags

  Ogre::Matrix4 localTrans{Ogre::Matrix4::IDENTITY};
  if (auto body = dynamic_cast<nif::bhk::RigidBodyT *>(block)) {
    using namespace engine::conversions;
    localTrans.makeTransform(
        fromBSCoordinates(fromNif(body->translation).xyz()),
        {1.0f, 1.0f, 1.0f},
        fromBSCoordinates(fromNif(body->rotation)));
    mLogger->trace(" * Applying RigidBodyT transform {}", localTrans);
  }
  mTransform = mTransform * localTrans;
  // TODO: RigidBody that is not a RigidBodyT

  auto[shape, shapeTag] = getRef<nif::bhk::Shape>(g, block->shape);
  auto collisionShape = parseShape(g, shape, shapeTag);

  mTransform = mTransform * localTrans.inverse();

  std::unique_ptr<Ogre::RigidBodyInfo> info{};
  if (auto body = dynamic_cast<nif::bhk::RigidBody *>(block)) {
    info = std::make_unique<Ogre::RigidBodyInfo>(generateRigidBodyInfo(body));
    info->m_collisionShape = collisionShape.get();
  }
  return std::make_pair(std::move(collisionShape), std::move(info));
}

Ogre::RigidBodyInfo
CollisionObjectVisitor::generateRigidBodyInfo(nif::bhk::RigidBody *block) const {
  using namespace engine::conversions;
  using namespace Ogre::conversions;

  // This does not seem to affect the translation in any way.
  // TODO: What is the Havok origin used for?
  Ogre::Vector3 origin = fromNif(block->center).xyz();

  // Bullet needs a diagonalized inertia tensor given as a vector, and the
  // file stores it as a 3x4 matrix. We ignore the last (w) column and
  // compute the eigenvalues. Also we need to change from BS coordinates.
  const auto &hkI = block->inertiaTensor;
  Ogre::Matrix3 bsI{hkI.m11, hkI.m12, hkI.m13,
                    hkI.m21, hkI.m22, hkI.m23,
                    hkI.m31, hkI.m32, hkI.m33};
  Ogre::Matrix3 inertiaTensor{fromBSCoordinates(bsI)};
  std::array<Ogre::Vector3, 3> principalAxes{};
  Ogre::Vector3 principalMoments{};
  inertiaTensor.EigenSolveSymmetric(principalMoments.ptr(),
                                    principalAxes.data());
  mLogger->trace(
      "Diagonalized inertia tensor, eigenvalues and eigenvectors are");
  for (int i = 0; i < 3; ++i) {
    mLogger->trace(" * {}, {}", principalMoments[i], principalAxes[i]);
  }

  // We have the diagonalization
  // inertiaTensor = principalAxes * diag(principalMoments) * principalAxes^T
  // To do this properly we should change to the principal frame but for now
  // assume the principal axes are coordinate-axis aligned (this seems to hold
  // in practice).
  // TODO: Change to principal frame

  // The units for these are the same in Bullet and Havok
  float mass = block->mass;
  float linearDamping = block->linearDamping;
  float angularDamping = block->angularDamping;
  float friction = block->friction;
  float restitution = block->restitution;

  // TODO: Constraints

  // Convert the Ogre parameters to Bullet ones and set up the rigid body
  btVector3 bulletInertia{toBullet(principalMoments)};
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
                                   nif::bhk::Shape *block,
                                   engine::nifloader::LoadStatus &tag) {
  if (auto moppBvTreeShape = dynamic_cast<nif::bhk::MoppBvTreeShape *>(block)) {
    engine::nifloader::Tagger tagger{tag};
    mLogger->trace("Parsing block ? (bhkMoppBvTreeShape)");
    mLogger->trace(" * New transform = {}", mTransform);

    auto material = moppBvTreeShape->material.material;

    // Instead of decoding the MOPP data we use the linked shape
    auto[shape, shapeTag] = getRef<nif::bhk::Shape>(g, moppBvTreeShape->shape);

    // Apply the scale and recurse into the linked shape
    // The effort here is to avoid scaling the w component
    float scale = moppBvTreeShape->shapeScale;
    Ogre::Matrix4 scaleMat{Ogre::Matrix4::IDENTITY};
    scaleMat.setScale({scale, scale, scale});
    mTransform = mTransform * scaleMat;
    auto collisionShape = parseShape(g, shape, shapeTag);
    // Don't forget to undo the transform
    mTransform = mTransform * scaleMat.inverse();
    return collisionShape;
  } else if (auto niTriStrips =
      dynamic_cast<nif::bhk::PackedNiTriStripsShape *>(block)) {
    engine::nifloader::Tagger tagger{tag};
    mLogger->trace("Parsing block ? (bhkPackedNiTriStripsShape)");
    mLogger->trace(" * New transform = {}", mTransform);

    // TODO: Subshapes?

    // @formatter:off
    auto[data, dataTag] =
        getRef<nif::hk::PackedNiTriStripsData>(g, niTriStrips->data);
    // @formatter:on

    Ogre::Matrix4 scaleMat{Ogre::Matrix4::IDENTITY};
    // For some reason the coordinates are scaled down in the nif file by a
    // factor of 7. This scale needs to also apply to the translation, not
    // just the linear part.
    scaleMat.setScale(engine::conversions::fromNif(niTriStrips->scale).xyz());
    mTransform = mTransform * scaleMat * 7.0f;
    auto collisionShape = parseNiTriStripsData(g, data, dataTag);
    mTransform = mTransform * scaleMat.inverse() * (1.0f / 7.0f);
    return collisionShape;
  } else if (auto convexVerticesShape =
      dynamic_cast<nif::bhk::ConvexVerticesShape *>(block)) {
    engine::nifloader::Tagger tagger{tag};
    mLogger->trace("Parsing block ? (bhkConvexVerticesShape)");
    mLogger->trace(" * New transform = {}", mTransform);

    auto material = convexVerticesShape->material.material;

    auto collisionShape = std::make_unique<btConvexHullShape>();
    for (const auto &vertex : convexVerticesShape->vertices) {
      using namespace engine::conversions;
      using namespace Ogre::conversions;
      Ogre::Vector4 ogreV{fromBSCoordinates(fromNif(vertex).xyz()), 1.0f};
      auto v = mTransform * ogreV * 7.0f;
      collisionShape->addPoint(toBullet(v.xyz()));
    }
    return collisionShape;
  } else if (auto boxShape = dynamic_cast<nif::bhk::BoxShape *>(block)) {
    engine::nifloader::Tagger tagger{tag};
    mLogger->trace("Parsing block ? (bhkBoxShape)");
    mLogger->trace(" * New transform = {}", mTransform);

    auto material = boxShape->material.material;

    // Applying the nif transform may result in a non-axis-aligned box, which
    // btBoxShape does not support, so we use a btConvexHullShape instead.
    // If necessary, one could check that the box stays axis-aligned by
    // extracting the rotation and comparing the volumes of the original
    // axis-aligned box and the rotated axis-aligned box.
    using namespace engine::conversions;
    using namespace Ogre::conversions;
    auto collisionShape = std::make_unique<btConvexHullShape>();
    Ogre::Vector3 halfExtents{fromNif(boxShape->dimensions).xyz()};
    Ogre::AxisAlignedBox box{halfExtents, halfExtents};
    for (const auto &corner : box.getAllCorners()) {
      Ogre::Vector4 ogreV{fromBSCoordinates(corner), 1.0f};
      auto v = mTransform * ogreV * 7.0f;
      collisionShape->addPoint(toBullet(v.xyz()));
    }
    return collisionShape;
  } else {
    engine::nifloader::Tagger tagger{tag};
    mLogger->warn("Parsing unknown bhkShape");
    return nullptr;
    //OGRE_EXCEPT(Ogre::Exception::ERR_NOT_IMPLEMENTED,
    //            "Unknown collision shape",
    //            "CollisionObjectVisitor::parseShape");
  }
}
std::unique_ptr<btCollisionShape>
CollisionObjectVisitor::parseNiTriStripsData(const Graph &g,
                                             nif::hk::PackedNiTriStripsData *block,
                                             engine::nifloader::LoadStatus &tag) {
  engine::nifloader::Tagger tagger{tag};
  mLogger->trace("Parsing block ? (hkPackedNiTriStripsData)");
  mLogger->trace(" * New transform = {}", mTransform);

  // For static geometry we construct a btBvhTriangleMeshShape using indexed
  // triangles. Bullet doesn't copy the underlying vertex and index buffers,
  // so they need to be kept alive for the lifetime of the collision object.
  btIndexedMesh indexedMesh{};

  indexedMesh.m_numTriangles = block->numTriangles;
  indexedMesh.m_numVertices = block->numVertices;

  // TODO: Is aligning the triangles to 8-byte boundaries faster than packing?
  indexedMesh.m_triangleIndexStride = 3 * sizeof(nif::basic::UShort);

  // Vertex data is always in single-precision, regardless of Ogre or Bullet
  indexedMesh.m_vertexType = PHY_FLOAT;
  indexedMesh.m_vertexStride = 3 * sizeof(float);

  // Copy into index buffer
  auto &indexBuffer = mRigidBody->mIndexBuffer;
  indexBuffer.assign(indexedMesh.m_numTriangles * 3u, 0u);
  {
    auto it = indexBuffer.begin();
    for (const auto &triData : block->triangles) {
      const auto &tri = triData.triangle;
      *it = tri.v1;
      *(it + 1) = tri.v2;
      *(it + 2) = tri.v3;
      it += 3;
    }
  }
  indexedMesh.m_triangleIndexBase =
      reinterpret_cast<unsigned char *>(indexBuffer.data());

  // Copy into vertex buffer
  auto &vertexBuffer = mRigidBody->mVertexBuffer;
  vertexBuffer.assign(indexedMesh.m_numVertices * 3u, 0.0f);
  {
    auto it = vertexBuffer.begin();
    for (const auto &vertex : block->vertices) {
      using namespace engine::conversions;
      Ogre::Vector4 ogreV{fromBSCoordinates(fromNif(vertex))};
      auto v = mTransform * ogreV;
      *it = v.x;
      *(it + 1) = v.y;
      *(it + 2) = v.z;
      it += 3;
    }
  }
  indexedMesh.m_vertexBase =
      reinterpret_cast<unsigned char *>(vertexBuffer.data());

  // Construct the actual mesh and give ownership to the rigid body
  auto collisionMesh = std::make_unique<btTriangleIndexVertexArray>();
  collisionMesh->addIndexedMesh(indexedMesh, PHY_SHORT);
  mRigidBody->mCollisionMesh = std::move(collisionMesh);

  return std::make_unique<btBvhTriangleMeshShape>(
      mRigidBody->mCollisionMesh.get(), false);

  // TODO: Support dynamic concave geometry

  return nullptr;
}

} // namespace engine::nifloader