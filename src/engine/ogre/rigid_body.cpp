#include "engine/conversions.hpp"
#include "engine/nifloader/loader.hpp"
#include "engine/ogre/rigid_body.hpp"
#include "engine/ogre/ogre_stream_wrappers.hpp"
#include "engine/settings.hpp"
#include "nif/bhk.hpp"
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/copy.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <btBulletDynamicsCommon.h>
#include <Ogre.h>
#include <OgreResourceGroupManager.h>
#include <spdlog/spdlog.h>
#include <array>

namespace Ogre {

RigidBody::RigidBody(Ogre::ResourceManager *creator,
                     const Ogre::String &name,
                     Ogre::ResourceHandle handle,
                     const Ogre::String &group,
                     bool isManual,
                     Ogre::ManualResourceLoader *loader)
    : Resource(creator, name, handle, group, isManual, loader) {}

btRigidBody *RigidBody::getRigidBody() {
  return mRigidBody.get();
}

btCollisionShape *RigidBody::getCollisionShape() {
  return mCollisionShape.get();
}

void RigidBody::bind(SceneNode *node) {
  // Reset and delete existing state
  if (mRigidBody) mRigidBody->setMotionState(nullptr);
  mMotionState.reset(nullptr);

  // Allocate and set new state
  if (node) {
    mMotionState = std::make_unique<MotionState>(node);
    if (mRigidBody) mRigidBody->setMotionState(mMotionState.get());
  }
}

void RigidBody::notify() {
  if (mMotionState) mMotionState->notify();
}

void RigidBody::loadImpl() {
  spdlog::get(engine::settings::ogreLog)->info("RigidBody: {}", getName());

  auto ogreDataStream = ResourceGroupManager::getSingleton()
      .openResource(mName, mGroup);

  if (ogreDataStream == nullptr) {
    OGRE_EXCEPT(Exception::ERR_FILE_NOT_FOUND,
                "Unable to open resource",
                "RigidBody::load");
  }

  auto ogreDataStreambuf = engine::OgreDataStreambuf{ogreDataStream};
  std::istream is{&ogreDataStreambuf};

  auto untaggedBlocks = engine::nifloader::createBlockGraph(is);
  RigidBodyNifVisitor::Graph blocks{};
  boost::copy_graph(untaggedBlocks, blocks);

  std::vector<boost::default_color_type> colorMap(boost::num_vertices(blocks));
  auto propertyMap = boost::make_iterator_property_map(
      colorMap.begin(), boost::get(boost::vertex_index, blocks));

  boost::depth_first_search(blocks, RigidBodyNifVisitor(this), propertyMap);

  if (mMotionState) mRigidBody->setMotionState(mMotionState.get());
}

void RigidBody::unloadImpl() {
  // TODO: Actually unload the thing
}

void RigidBodyNifVisitor::start_vertex(vertex_descriptor v, const Graph &g) {
  mTransform = Ogre::Matrix4::IDENTITY;
}

void RigidBodyNifVisitor::discover_vertex(vertex_descriptor v, const Graph &g) {
  auto &taggedNiObject = g[v];
  auto *niObject = taggedNiObject.block.get();
  auto &tag = taggedNiObject.tag;

  if (auto niNode = dynamic_cast<nif::NiNode *>(niObject)) {
    mTransform = mTransform * engine::nifloader::getTransform(niNode);
  } else if (auto bsxFlags = dynamic_cast<nif::BSXFlags *>(niObject)) {
    using Flags = nif::BSXFlags::Flags;
    auto flags = Flags(bsxFlags->data);
    if ((flags & Flags::bHavok) != Flags::bNone) {
      mHasHavok = true;
    }
  } else if (auto collisionObject =
      dynamic_cast<nif::bhk::CollisionObject *>(niObject)) {
    if (!mHasHavok) return;
    parseCollisionObject(g, collisionObject, tag);
  } else if (auto rigidBody = dynamic_cast<nif::bhk::RigidBody *>(niObject)) {
    if (!mHasHavok) return;
    // RigidBody ignores the translations and rotations of its parent, whereas
    // RigidBodyT does not ignore them.
    bool ignoreRT = (dynamic_cast<nif::bhk::RigidBodyT *>(niObject) == nullptr);
  }
}

void RigidBodyNifVisitor::finish_vertex(vertex_descriptor v, const Graph &g) {
  auto *niObject = g[v].block.get();
  if (auto niNode = dynamic_cast<nif::NiNode *>(niObject)) {
    mTransform = mTransform * engine::nifloader::getTransform(niNode).inverse();
  }
}

void RigidBodyNifVisitor::parseCollisionObject(const Graph &g,
                                               nif::bhk::CollisionObject *block,
                                               engine::nifloader::LoadStatus &tag) {
  // TODO: COFlags
  // TODO: target
  auto[worldObj, worldObjTag] = getRef<nif::bhk::WorldObject>(g, block->body);
  parseWorldObject(g, worldObj, worldObjTag);
}

void RigidBodyNifVisitor::parseWorldObject(const Graph &g,
                                           nif::bhk::WorldObject *block,
                                           engine::nifloader::LoadStatus &tag) {
  // TODO: Flags
  auto[shape, shapeTag] = getRef<nif::bhk::Shape>(g, block->shape);
  mRigidBody->mCollisionShape = parseShape(g, shape, shapeTag);

  if (auto rigidBody = dynamic_cast<nif::bhk::RigidBody *>(block)) {
    using namespace engine::conversions;
    // RigidBody ignores the translations and rotations of its parent, whereas
    // RigidBodyT does not ignore them.
    bool ignoreRT = (dynamic_cast<nif::bhk::RigidBodyT *>(block) == nullptr);
    Matrix4 baseTrans{ignoreRT ? Matrix4::IDENTITY : mTransform};

    // Transformation of the body is always taken into account
    Matrix4 localTrans{};
    localTrans.makeTransform(
        fromNif(rigidBody->translation).xyz(),
        {1.0f, 1.0f, 1.0f},
        fromNif(rigidBody->rotation));
    auto totalTrans = baseTrans * localTrans;

    // This does not seem to affect the translation in any way.
    // TODO: What is the Havok origin used for?
    Vector3 origin = fromNif(rigidBody->center).xyz();

    // Bullet needs a diagonalized inertia tensor given as a vector, and the
    // file stores it as a 3x4 matrix. We ignore the last (w) column and
    // compute the eigenvalues.
    const auto &hkI = rigidBody->inertiaTensor;
    Matrix3 inertiaTensor{hkI.m11, hkI.m12, hkI.m13,
                          hkI.m21, hkI.m22, hkI.m23,
                          hkI.m31, hkI.m32, hkI.m33};
    std::array<Vector3, 3> principalAxes{};
    Vector3 principalMoments{};
    inertiaTensor.EigenSolveSymmetric(principalMoments.ptr(),
                                      principalAxes.data());
    // The principal axes for a rotation matrix and we have the diagonalization
    // inertiaTensor = principalAxes * diag(principalMoments) * principalAxes^T
    // TODO: Change to principal frame

    // The units for these are the same in Bullet and Havok
    btScalar mass = rigidBody->mass;
    btScalar linearDamping = rigidBody->linearDamping;
    btScalar angularDamping = rigidBody->angularDamping;
    btScalar friction = rigidBody->friction;
    btScalar restitution = rigidBody->restitution;

    // TODO: Constraints

    // Convert the Ogre parameters to Bullet ones and set up the rigid body
    btVector3 bulletInertia{toBullet(principalMoments)};
    btRigidBody::btRigidBodyConstructionInfo
        info(mass, nullptr, mRigidBody->mCollisionShape.get(), bulletInertia);
    info.m_linearDamping = linearDamping;
    info.m_angularDamping = angularDamping;
    info.m_friction = friction;
    info.m_restitution = restitution;
    info.m_startWorldTransform = btTransform{toBullet(totalTrans.linear()),
                                             toBullet(totalTrans.getTrans())};

    mRigidBody->mRigidBody = std::make_unique<btRigidBody>(info);
  }
}

std::unique_ptr<btCollisionShape>
RigidBodyNifVisitor::parseShape(const Graph &g,
                                nif::bhk::Shape *block,
                                engine::nifloader::LoadStatus &tag) {
  if (auto moppBvTreeShape = dynamic_cast<nif::bhk::MoppBvTreeShape *>(block)) {
    auto material = moppBvTreeShape->material.material;

    // Instead of decoding the MOPP data we use the linked shape
    auto[shape, shapeTag] = getRef<nif::bhk::Shape>(g, moppBvTreeShape->shape);

    // Apply the scale and recurse into the linked shape
    // The effort here is to avoid scaling the w component
    Real scale = moppBvTreeShape->shapeScale;
    Matrix4 scaleMat{};
    scaleMat.setScale({scale, scale, scale});
    mTransform = mTransform * scaleMat;
    auto collisionShape = parseShape(g, shape, shapeTag);
    // Don't forget to undo the transform
    mTransform = mTransform * scaleMat.inverse();
    return collisionShape;
  } else if (auto niTriStrips =
      dynamic_cast<nif::bhk::PackedNiTriStripsShape *>(block)) {
    // TODO: Subshapes?

    // @formatter:off
    auto[data, dataTag] =
        getRef<nif::hk::PackedNiTriStripsData>(g, niTriStrips->data);
    // @formatter:on

    Matrix4 scaleMat{};
    // For some reason the coordinates are scaled down in the nif file by a
    // factor of 7.
    scaleMat.setScale(
        7.0f * engine::conversions::fromNif(niTriStrips->scale).xyz());
    mTransform = mTransform * scaleMat;
    auto collisionShape = parseNiTriStripsData(g, data, dataTag);
    mTransform = mTransform * scaleMat.inverse();
    return collisionShape;
  } else {
    return nullptr;
    //OGRE_EXCEPT(Exception::ERR_NOT_IMPLEMENTED,
    //            "Unknown collision shape",
    //            "RigidBodyNifVisitor::parseShape");
  }
}

std::unique_ptr<btCollisionShape>
RigidBodyNifVisitor::parseNiTriStripsData(const Graph &g,
                                          nif::hk::PackedNiTriStripsData *block,
                                          engine::nifloader::LoadStatus &tag) {
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
      Vector4 ogreV{fromBSCoordinates(fromNif(vertex))};
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

} // namespace Ogre