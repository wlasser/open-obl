#include "ogrebullet/collision_object.hpp"
#include <btBulletDynamicsCommon.h>
#include <Ogre.h>
#include <OgreResourceGroupManager.h>

namespace Ogre {

CollisionObject::CollisionObject(Ogre::ResourceManager *creator,
                                 const Ogre::String &name,
                                 Ogre::ResourceHandle handle,
                                 const Ogre::String &group,
                                 bool isManual,
                                 Ogre::ManualResourceLoader *loader)
    : Resource(creator, name, handle, group, isManual, loader) {}

const RigidBodyInfo *CollisionObject::getRigidBodyInfo() const noexcept {
  return mInfo.get();
}

const btCollisionShape *CollisionObject::getCollisionShape() const noexcept {
  return mCollisionShape.get();
}

void CollisionObject::loadImpl() {
  OGRE_EXCEPT(Exception::ERR_NOT_IMPLEMENTED,
              "There is no default loader, please supply a ManualResourceLoader",
              "CollisionObject::load()");
}

void CollisionObject::unloadImpl() {
  mInfo.reset();
  mCollisionShape.reset();
  mIndexBuffer.clear();
  mIndexBuffer.shrink_to_fit();
  mVertexBuffer.clear();
  mVertexBuffer.shrink_to_fit();
  mMeshInterface.reset();
}

void CollisionObject::_setRigidBodyInfo(
    std::unique_ptr<RigidBodyInfo> info) noexcept {
  mInfo = std::move(info);
}

void CollisionObject::_setCollisionShape(
    std::unique_ptr<btCollisionShape> shape) noexcept {
  mCollisionShape = std::move(shape);
}

void CollisionObject::_storeIndirectCollisionShapes(
    std::vector<CollisionShapePtr> shapes) noexcept {
  mIndirectShapes = std::move(shapes);
}

void CollisionObject::_setMeshInterface(
    std::unique_ptr<btStridingMeshInterface> mesh) noexcept {
  mMeshInterface = std::move(mesh);
}

std::vector<uint16_t> &CollisionObject::_getIndexBuffer() noexcept {
  return mIndexBuffer;
}

std::vector<float> &CollisionObject::_getVertexBuffer() noexcept {
  return mVertexBuffer;
}

} // namespace Ogre