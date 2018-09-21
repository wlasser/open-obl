#ifndef OPENOBLIVION_ENGINE_RIGID_BODY_HPP
#define OPENOBLIVION_ENGINE_RIGID_BODY_HPP

#include <btBulletDynamicsCommon.h>
#include <OgreResource.h>

namespace Ogre {

class RigidBody : public Ogre::Resource {
 public:
  RigidBody(ResourceManager *creator,
            const String &name,
            ResourceHandle handle,
            const String &group,
            bool isManual = false,
            ManualResourceLoader *loader = nullptr);

  ~RigidBody() override {
    unload();
  }

  btRigidBody *getRigidBody();

 protected:
  void loadImpl() override;
  void unloadImpl() override;

 private:
  // TODO: Use custom allocators
  std::unique_ptr<btRigidBody> mRigidBody{};
};

using RigidBodyPtr = std::shared_ptr<RigidBody>;

} // namespace Ogre

#endif // OPENOBLIVION_ENGINE_RIGID_BODY_HPP
