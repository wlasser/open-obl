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

RigidBodyInfo *CollisionObject::getRigidBodyInfo() const {
  return mInfo.get();
}

btCollisionShape *CollisionObject::getCollisionShape() const {
  return mCollisionShape.get();
}

void CollisionObject::loadImpl() {
  // This needs to be implemented by a ManualResourceLoader
}

void CollisionObject::unloadImpl() {
  // TODO: Actually unload the thing
}

} // namespace Ogre