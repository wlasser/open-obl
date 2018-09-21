#include "engine/ogre/rigid_body_manager.hpp"
#include <OgreResourceGroupManager.h>

namespace Ogre {

template<> RigidBodyManager *Singleton<RigidBodyManager>::msSingleton = nullptr;

RigidBodyManager &RigidBodyManager::getSingleton() {
  assert(msSingleton);
  return *msSingleton;
}

RigidBodyManager *RigidBodyManager::getSingletonPtr() {
  return msSingleton;
}

RigidBodyManager::RigidBodyManager() {
  mLoadOrder = 400.0f;
  mResourceType = "RigidBody";
  ResourceGroupManager::getSingleton()
      ._registerResourceManager(mResourceType, this);
}

RigidBodyManager::~RigidBodyManager() {
  ResourceGroupManager::getSingleton()
      ._unregisterResourceManager(mResourceType);
}

// The memory management requires an explanation. Calling createResource or
// getResourceByName in the base calls createImpl in the derived. This allocates
// memory and returns a non-owning pointer, but is converted into a
// shared_ptr<Resource> by the caller. Since we know that createImpl
// actually creates a RigidBody*, not a Resource*, we are free to statically
// downcast the shared_ptr<Resource> returned by createResource or
// getResourceByName into a shared_ptr<RigidBody>.

RigidBodyPtr RigidBodyManager::create(const Ogre::String &name,
                                      const Ogre::String &group,
                                      bool isManual,
                                      Ogre::ManualResourceLoader *loader,
                                      const Ogre::NameValuePairList *createParams) {
  return static_pointer_cast<RigidBody>(
      createResource(name, group, isManual, loader, createParams));
}

RigidBodyPtr RigidBodyManager::getByName(const Ogre::String &name,
                                         const Ogre::String &group) {
  return static_pointer_cast<RigidBody>(getResourceByName(name, group));
}

Resource *RigidBodyManager::createImpl(const Ogre::String &name,
                                       Ogre::ResourceHandle handle,
                                       const Ogre::String &group,
                                       bool isManual,
                                       Ogre::ManualResourceLoader *loader,
                                       const Ogre::NameValuePairList *params) {
  return OGRE_NEW RigidBody(this, name, handle, group, isManual, loader);
}

} // namespace Ogre