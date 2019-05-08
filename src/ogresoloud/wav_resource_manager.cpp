#include "ogresoloud/wav_resource_manager.hpp"
#include <OgreResourceGroupManager.h>

namespace Ogre {

template<>
WavResourceManager *Singleton<WavResourceManager>::msSingleton = nullptr;

WavResourceManager &WavResourceManager::getSingleton() {
  assert(msSingleton);
  return *msSingleton;
}

WavResourceManager *WavResourceManager::getSingletonPTr() {
  return msSingleton;
}

WavResourceManager::WavResourceManager() {
  mLoadOrder = 550.0f;
  mResourceType = "Wav";
  auto &resGrpMgr{ResourceGroupManager::getSingleton()};
  resGrpMgr._registerResourceManager(mResourceType, this);
}

WavResourceManager::~WavResourceManager() {
  auto &resGrpMgr{ResourceGroupManager::getSingleton()};
  resGrpMgr._unregisterResourceManager(mResourceType);
}

WavResourcePtr WavResourceManager::create(const Ogre::String &name,
                                          const Ogre::String &group,
                                          bool isManual,
                                          Ogre::ManualResourceLoader *loader,
                                          const Ogre::NameValuePairList *createParams) {
  return static_pointer_cast<WavResource>(
      createResource(name, group, isManual, loader, createParams));
}

WavResourcePtr WavResourceManager::getByName(const Ogre::String &name,
                                             const Ogre::String &group) {
  return static_pointer_cast<WavResource>(getResourceByName(name, group));
}

Resource *WavResourceManager::createImpl(const Ogre::String &name,
                                         Ogre::ResourceHandle handle,
                                         const Ogre::String &group,
                                         bool isManual,
                                         Ogre::ManualResourceLoader *loader,
                                         const Ogre::NameValuePairList *) {
  return OGRE_NEW WavResource(this, name, handle, group, isManual, loader);
}

} // namespace Ogre