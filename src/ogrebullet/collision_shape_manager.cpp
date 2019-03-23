#include "ogrebullet/collision_shape_manager.hpp"
#include <OgreResourceGroupManager.h>

namespace Ogre {

template<> CollisionShapeManager
    *Singleton<CollisionShapeManager>::msSingleton = nullptr;

CollisionShapeManager &CollisionShapeManager::getSingleton() {
  assert(msSingleton);
  return *msSingleton;
}

CollisionShapeManager *CollisionShapeManager::getSingletonPtr() {
  return msSingleton;
}

CollisionShapeManager::CollisionShapeManager() {
  mLoadOrder = 400.0f;
  mResourceType = "CollisionShape";
  ResourceGroupManager::getSingleton()
      ._registerResourceManager(mResourceType, this);
}

CollisionShapeManager::~CollisionShapeManager() {
  ResourceGroupManager::getSingleton()
      ._unregisterResourceManager(mResourceType);
}

CollisionShapePtr
CollisionShapeManager::create(const Ogre::String &name,
                               const Ogre::String &group,
                               bool isManual,
                               Ogre::ManualResourceLoader *loader,
                               const Ogre::NameValuePairList *createParams) {
  return static_pointer_cast<CollisionShape>(
      createResource(name, group, isManual, loader, createParams));
}

CollisionShapePtr
CollisionShapeManager::getByName(const Ogre::String &name,
                                  const Ogre::String &group) {
  return static_pointer_cast<CollisionShape>(getResourceByName(name, group));
}

Resource *
CollisionShapeManager::createImpl(const Ogre::String &name,
                                  Ogre::ResourceHandle handle,
                                  const Ogre::String &group,
                                  bool isManual,
                                  Ogre::ManualResourceLoader *loader,
                                  const Ogre::NameValuePairList */*params*/) {
  return OGRE_NEW CollisionShape(this, name, handle, group, isManual, loader);
}

} // namespace Ogre