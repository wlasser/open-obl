#include "ogrebullet/collision_shape.hpp"
#include <btBulletDynamicsCommon.h>
#include <Ogre.h>
#include <OgreResourceGroupManager.h>

namespace Ogre {

CollisionShape::CollisionShape(Ogre::ResourceManager *creator,
                               const Ogre::String &name,
                               Ogre::ResourceHandle handle,
                               const Ogre::String &group,
                               bool isManual,
                               Ogre::ManualResourceLoader *loader)
    : Resource(creator, name, handle, group, isManual, loader) {}

CollisionShape::CollisionObjectType
CollisionShape::getCollisionObjectType() const noexcept {
  return mCollisionObjectType;
}

void CollisionShape::setCollisionObjectType(CollisionObjectType type) noexcept {
  mCollisionObjectType = type;
}

bool CollisionShape::getAllowDeactivationEnabled() const noexcept {
  return mAllowDeactivation;
}

void CollisionShape::setAllowDeactivationEnabled(bool enabled) noexcept {
  mAllowDeactivation = enabled;
}

const RigidBodyInfo *CollisionShape::getRigidBodyInfo() const noexcept {
  return mInfo.get();
}

const btCollisionShape *CollisionShape::getCollisionShape() const noexcept {
  return mCollisionShape.get();
}

void CollisionShape::loadImpl() {
  OGRE_EXCEPT(Exception::ERR_NOT_IMPLEMENTED,
              "There is no default loader, please supply a "
              "ManualResourceLoader",
              "CollisionShape::load()");
}

void CollisionShape::unloadImpl() {
  mInfo.reset();
  mCollisionShape.reset();
  mIndexBuffer.clear();
  mIndexBuffer.shrink_to_fit();
  mVertexBuffer.clear();
  mVertexBuffer.shrink_to_fit();
  mMeshInterface.reset();
}

void CollisionShape::_setRigidBodyInfo(
    std::unique_ptr<RigidBodyInfo> info) noexcept {
  mInfo = std::move(info);
}

void CollisionShape::_setCollisionShape(
    std::unique_ptr<btCollisionShape> shape) noexcept {
  mCollisionShape = std::move(shape);
}

void CollisionShape::_storeIndirectCollisionShapes(
    std::vector<BulletCollisionShapePtr> shapes) noexcept {
  mIndirectShapes = std::move(shapes);
}

void CollisionShape::_setMeshInterface(
    std::unique_ptr<btStridingMeshInterface> mesh) noexcept {
  mMeshInterface = std::move(mesh);
}

std::vector<uint16_t> &CollisionShape::_getIndexBuffer() noexcept {
  return mIndexBuffer;
}

std::vector<float> &CollisionShape::_getVertexBuffer() noexcept {
  return mVertexBuffer;
}

} // namespace Ogre