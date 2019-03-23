#ifndef OPENOBLIVION_OGREBULLET_COLLISION_SHAPE_MANAGER_HPP
#define OPENOBLIVION_OGREBULLET_COLLISION_SHAPE_MANAGER_HPP

#include "ogrebullet/collision_shape.hpp"
#include <btBulletDynamicsCommon.h>
#include <OgreResourceManager.h>
#include <OgreSingleton.h>

namespace Ogre {

/// `Ogre::ResourceManager` for `Ogre::CollisionShape`.
/// This doesn't do anything fancy, it is basically just a boilerplate
/// `Ogre::ResourceManager`. For reference, here's a description of the memory
/// management for `Ogre::ResourceManager`s:
/// > Calling `createResource()` or `getResourceByName()` in the base class
/// > calls `createImpl()` in the derived class. This allocates memory, usually
/// > via `OGRE_NEW`, and returns a non-owning pointer that is used to construct
/// > a `std::shared_ptr<Ogre::Resource>`, allowing the caller to take
/// > ownership. In particular, `OGRE_FREE` is never called explicitly because
/// > the`std::shared_ptr` now takes manages the memory. This owning pointer is
/// > given back to us in `create()` and `getByName()`, where we can safely
/// > `std::static_pointer_cast` it to a `std::shared_ptr` of the derived
/// > resource, and return.
class CollisionShapeManager : public ResourceManager,
                              public Singleton<CollisionShapeManager> {
 public:
  CollisionShapeManager();
  ~CollisionShapeManager() override;

  CollisionShapePtr create(const String &name,
                           const String &group,
                           bool isManual = false,
                           ManualResourceLoader *loader = nullptr,
                           const NameValuePairList *createParams = nullptr);

  CollisionShapePtr getByName(const String &name,
                              const String &group OGRE_RESOURCE_GROUP_INIT);

  static CollisionShapeManager &getSingleton();
  static CollisionShapeManager *getSingletonPtr();

 protected:
  Resource *createImpl(const String &name,
                       ResourceHandle handle,
                       const String &group,
                       bool isManual,
                       ManualResourceLoader *loader,
                       const NameValuePairList *params) override;
};

} // namespace Ogre

#endif // OPENOBLIVION_OGREBULLET_COLLISION_SHAPE_MANAGER_HPP
