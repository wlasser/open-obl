#ifndef OPENOBLIVION_OGREBULLET_COLLISION_OBJECT_MANAGER_HPP
#define OPENOBLIVION_OGREBULLET_COLLISION_OBJECT_MANAGER_HPP

#include "ogrebullet/collision_object.hpp"
#include <btBulletDynamicsCommon.h>
#include <OgreResourceManager.h>
#include <OgreSingleton.h>

namespace Ogre {

class CollisionObjectManager : public ResourceManager,
                               public Singleton<CollisionObjectManager> {
 public:
  CollisionObjectManager();
  ~CollisionObjectManager() override;

  CollisionObjectPtr create(const String &name,
                      const String &group,
                      bool isManual = false,
                      ManualResourceLoader *loader = nullptr,
                      const NameValuePairList *createParams = nullptr);

  CollisionObjectPtr getByName(const String &name,
                               const String &group OGRE_RESOURCE_GROUP_INIT);

  static CollisionObjectManager &getSingleton();
  static CollisionObjectManager *getSingletonPtr();

 protected:
  Resource *createImpl(const String &name,
                       ResourceHandle handle,
                       const String &group,
                       bool isManual,
                       ManualResourceLoader *loader,
                       const NameValuePairList *params) override;
};

} // namespace Ogre

#endif // OPENOBLIVION_OGREBULLET_COLLISION_OBJECT_MANAGER_HPP
