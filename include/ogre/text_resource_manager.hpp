#ifndef OPENOBLIVION_OGRE_TEXT_RESOURCE_MANAGER_HPP
#define OPENOBLIVION_OGRE_TEXT_RESOURCE_MANAGER_HPP

#include "ogre/text_resource.hpp"
#include <OgrePrerequisites.h>
#include <OgreResource.h>
#include <OgreResourceManager.h>
#include <OgreSingleton.h>

namespace Ogre {

class TextResourceManager : public ResourceManager,
                            public Singleton<TextResourceManager> {
 public:
  TextResourceManager();
  ~TextResourceManager() override;

  TextResourcePtr create(const String &name,
                         const String &group,
                         bool isManual = false,
                         ManualResourceLoader *loader = nullptr,
                         const NameValuePairList *createParams = nullptr);

  TextResourcePtr getByName(const String &name,
                            const String &group OGRE_RESOURCE_GROUP_INIT);

  static TextResourceManager &getSingleton();
  static TextResourceManager *getSingletonPtr();

 protected:
  Resource *createImpl(const String &name,
                       ResourceHandle,
                       const String &group,
                       bool isManual,
                       ManualResourceLoader *loader,
                       const NameValuePairList *params) override;
};

} // namespace Ogre

#endif // OPENOBLIVION_OGRE_TEXT_RESOURCE_MANAGER_HPP
