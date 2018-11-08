#include "nifloader/nif_resource_manager.hpp"
#include <OgreResourceGroupManager.h>

namespace Ogre {

template<> NifResourceManager
    *Singleton<NifResourceManager>::msSingleton = nullptr;

NifResourceManager &NifResourceManager::getSingleton() {
  assert(msSingleton);
  return *msSingleton;
}

NifResourceManager *NifResourceManager::getSingletonPtr() {
  return msSingleton;
}

NifResourceManager::NifResourceManager() {
  mLoadOrder = 300.0f;
  mResourceType = "Nif";
  ResourceGroupManager::getSingleton()
      ._registerResourceManager(mResourceType, this);
}

NifResourceManager::~NifResourceManager() {
  ResourceGroupManager::getSingleton()
      ._unregisterResourceManager(mResourceType);
}

NifResourcePtr
NifResourceManager::create(const String &name,
                           const String &group,
                           bool isManual,
                           ManualResourceLoader *loader,
                           const NameValuePairList *createParams) {
  return std::static_pointer_cast<NifResource>(
      createResource(name, group, isManual, loader, createParams));
}

NifResourcePtr NifResourceManager::getByName(const String &name,
                                             const String &group) {
  return std::static_pointer_cast<NifResource>(getResourceByName(name, group));
}

Resource *NifResourceManager::createImpl(const String &name,
                                         ResourceHandle handle,
                                         const String &group,
                                         bool isManual,
                                         ManualResourceLoader *loader,
                                         const NameValuePairList *params) {
  return OGRE_NEW NifResource(this, name, handle, group, isManual, loader);
}

} // namespace Ogre
