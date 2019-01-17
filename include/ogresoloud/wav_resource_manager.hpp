#ifndef OPENOBLIVION_OGRESOLOUD_WAV_RESOURCE_MANAGER_HPP
#define OPENOBLIVION_OGRESOLOUD_WAV_RESOURCE_MANAGER_HPP

#include "ogresoloud/wav_resource.hpp"
#include <OgreResource.h>
#include <OgreResourceManager.h>

namespace Ogre {

class WavResourceManager : public ResourceManager,
                           public Singleton<WavResourceManager> {
 public:
  explicit WavResourceManager();
  ~WavResourceManager() override;

  WavResourcePtr create(const String &name,
                        const String &group,
                        bool isManual = false,
                        ManualResourceLoader *loader = nullptr,
                        const NameValuePairList *createParams = nullptr);

  WavResourcePtr getByName(const String &name,
                           const String &group OGRE_RESOURCE_GROUP_INIT);

  static WavResourceManager &getSingleton();
  static WavResourceManager *getSingletonPTr();

 protected:
  Resource *createImpl(const String &name,
                       ResourceHandle,
                       const String &group,
                       bool isManual,
                       ManualResourceLoader *load,
                       const NameValuePairList *params) override;
};

} // namespace Ogre

#endif // OPENOBLIVION_OGRESOLOUD_WAV_RESOURCE_MANAGER_HPP
