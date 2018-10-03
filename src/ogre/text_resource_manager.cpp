#include "ogre/text_resource_manager.hpp"
#include <OgreResourceGroupManager.h>

namespace Ogre {

template<> TextResourceManager
    *Singleton<TextResourceManager>::msSingleton = nullptr;

TextResourceManager &TextResourceManager::getSingleton() {
  assert(msSingleton);
  return *msSingleton;
}

TextResourceManager *TextResourceManager::getSingletonPtr() {
  return msSingleton;
}

TextResourceManager::TextResourceManager() {
  mLoadOrder = 500.0f;
  mResourceType = "Text";
  ResourceGroupManager::getSingleton()
      ._registerResourceManager(mResourceType, this);
}

TextResourceManager::~TextResourceManager() {
  ResourceGroupManager::getSingleton()
      ._unregisterResourceManager(mResourceType);
}

TextResourcePtr TextResourceManager::create(const Ogre::String &name,
                                            const Ogre::String &group,
                                            bool isManual,
                                            Ogre::ManualResourceLoader *loader,
                                            const Ogre::NameValuePairList *createParams) {
  return static_pointer_cast<TextResource>(
      createResource(name, group, isManual, loader, createParams));
}

TextResourcePtr TextResourceManager::getByName(const Ogre::String &name,
                                               const Ogre::String &group) {
  return static_pointer_cast<TextResource>(getResourceByName(name, group));
}

Resource *TextResourceManager::createImpl(const Ogre::String &name,
                                          Ogre::ResourceHandle handle,
                                          const Ogre::String &group,
                                          bool isManual,
                                          Ogre::ManualResourceLoader *loader,
                                          const Ogre::NameValuePairList *params) {
  return OGRE_NEW TextResource(this, name, handle, group, isManual, loader);
}

} // namespace Ogre
