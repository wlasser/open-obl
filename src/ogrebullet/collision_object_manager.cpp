#include "ogrebullet/collision_object_manager.hpp"
#include <OgreResourceGroupManager.h>

namespace Ogre {

template<> CollisionObjectManager
    *Singleton<CollisionObjectManager>::msSingleton = nullptr;

CollisionObjectManager &CollisionObjectManager::getSingleton() {
  assert(msSingleton);
  return *msSingleton;
}

CollisionObjectManager *CollisionObjectManager::getSingletonPtr() {
  return msSingleton;
}

CollisionObjectManager::CollisionObjectManager() {
  mLoadOrder = 400.0f;
  mResourceType = "CollisionObject";
  ResourceGroupManager::getSingleton()
      ._registerResourceManager(mResourceType, this);
}

CollisionObjectManager::~CollisionObjectManager() {
  ResourceGroupManager::getSingleton()
      ._unregisterResourceManager(mResourceType);
}

// The memory management requires an explanation. Calling createResource or
// getResourceByName in the base calls createImpl in the derived. This allocates
// memory and returns a non-owning pointer, but is converted into a
// shared_ptr<Resource> by the caller. Since we know that createImpl
// actually creates a CollisionObject*, not a Resource*, we are free to
// statically downcast the shared_ptr<Resource> returned by createResource or
// getResourceByName into a shared_ptr<CollisionObject>.

CollisionObjectPtr
CollisionObjectManager::create(const Ogre::String &name,
                               const Ogre::String &group,
                               bool isManual,
                               Ogre::ManualResourceLoader *loader,
                               const Ogre::NameValuePairList *createParams) {
  return static_pointer_cast<CollisionObject>(
      createResource(name, group, isManual, loader, createParams));
}

CollisionObjectPtr
CollisionObjectManager::getByName(const Ogre::String &name,
                                  const Ogre::String &group) {
  return static_pointer_cast<CollisionObject>(getResourceByName(name, group));
}

Resource *
CollisionObjectManager::createImpl(const Ogre::String &name,
                                   Ogre::ResourceHandle handle,
                                   const Ogre::String &group,
                                   bool isManual,
                                   Ogre::ManualResourceLoader *loader,
                                   const Ogre::NameValuePairList */*params*/) {
  return OGRE_NEW CollisionObject(this, name, handle, group, isManual, loader);
}

} // namespace Ogre