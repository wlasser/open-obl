#ifndef OPENOBLIVION_OGREBULLET_COLLISION_OBJECT_HPP
#define OPENOBLIVION_OGREBULLET_COLLISION_OBJECT_HPP

#include "ogrebullet/motion_state.hpp"
#include <btBulletDynamicsCommon.h>
#include <OgreResource.h>
#include <memory>
#include <optional>

namespace Ogre {

using RigidBodyInfo = btRigidBody::btRigidBodyConstructionInfo;

class CollisionObject : public Ogre::Resource {
 public:
  CollisionObject(ResourceManager *creator,
                  const String &name,
                  ResourceHandle handle,
                  const String &group,
                  bool isManual = false,
                  ManualResourceLoader *loader = nullptr);

  ~CollisionObject() override {
    unload();
  }

  RigidBodyInfo *getRigidBodyInfo() const;

  btCollisionShape *getCollisionShape() const;

 protected:
  void loadImpl() override;
  void unloadImpl() override;

  // TODO: private:
 public:
  std::unique_ptr<RigidBodyInfo> mInfo{};

  // For performance reasons we don't want to duplicate the collision shape
  // for multiple instances of the same entity. Ideally therefore this would be
  // a non-owning pointer into a central store, which would store the collision
  // shape along with any necessary buffers.
  // TODO: Centralise the collision shapes and make this non-owning
  std::unique_ptr<btCollisionShape> mCollisionShape{};

  // Necessary for mesh-based collision shapes, Bullet does not take ownership.
  std::vector<uint16_t> mIndexBuffer{};
  std::vector<float> mVertexBuffer{};
  std::unique_ptr<btStridingMeshInterface> mCollisionMesh{};
};

using CollisionObjectPtr = std::shared_ptr<CollisionObject>;

} // namespace Ogre

#endif // OPENOBLIVION_OGREBULLET_COLLISION_OBJECT_HPP
