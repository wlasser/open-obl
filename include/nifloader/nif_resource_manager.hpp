#ifndef OPENOBLIVION_NIF_RESOURCE_MANAGER_HPP
#define OPENOBLIVION_NIF_RESOURCE_MANAGER_HPP

#include "nifloader/nif_resource.hpp"
#include <Ogre.h>

namespace Ogre {

/// \addtogroup OpenOblivionNifloader
/// @{

class NifResourceManager : public ResourceManager,
                           public Singleton<NifResourceManager> {
 public:
  NifResourceManager();
  ~NifResourceManager() override;

  NifResourcePtr create(const String &name,
                        const String &group,
                        bool isManual = false,
                        ManualResourceLoader *loader = nullptr,
                        const NameValuePairList *createParams = nullptr);

  NifResourcePtr getByName(const String &name,
                           const String &group OGRE_RESOURCE_GROUP_INIT);

  static NifResourceManager &getSingleton();
  static NifResourceManager *getSingletonPtr();

 protected:
  Resource *createImpl(const String &name,
                       ResourceHandle,
                       const String &group,
                       bool isManual,
                       ManualResourceLoader *loader,
                       const NameValuePairList *params) override;

};

/// @}

} // namespace Ogre

#endif // OPENOBLIVION_NIF_RESOURCE_MANAGER_HPP
