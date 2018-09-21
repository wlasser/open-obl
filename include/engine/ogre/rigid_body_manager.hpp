#ifndef OPENOBLIVION_ENGINE_RIGID_BODY_MANAGER_HPP
#define OPENOBLIVION_ENGINE_RIGID_BODY_MANAGER_HPP

#include "engine/ogre/rigid_body.hpp"
#include <btBulletDynamicsCommon.h>
#include <OgreResourceManager.h>
#include <OgreSingleton.h>

namespace Ogre {

class RigidBodyManager
    : public Ogre::ResourceManager, public Ogre::Singleton<RigidBodyManager> {
 public:
  RigidBodyManager();
  ~RigidBodyManager() override;

  RigidBodyPtr create(const String &name,
                      const String &group,
                      bool isManual = false,
                      ManualResourceLoader *loader = nullptr,
                      const NameValuePairList *createParams = nullptr);

  RigidBodyPtr getByName(const String &name,
                         const String &group OGRE_RESOURCE_GROUP_INIT);

  static RigidBodyManager &getSingleton();
  static RigidBodyManager *getSingletonPtr();

 protected:
  Resource *createImpl(const String &name,
                       ResourceHandle handle,
                       const String &group,
                       bool isManual,
                       ManualResourceLoader *loader,
                       const NameValuePairList *params) override;
};

} // namespace Ogre

#endif // OPENOBLIVION_ENGINE_RIGID_BODY_MANAGER_HPP
