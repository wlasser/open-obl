#ifndef OPENOBLIVION_OGREBULLET_COLLISION_SHAPE_HPP
#define OPENOBLIVION_OGREBULLET_COLLISION_SHAPE_HPP

#include "ogrebullet/motion_state.hpp"
#include <btBulletDynamicsCommon.h>
#include <OgreResource.h>
#include <memory>
#include <optional>

namespace Ogre {

using RigidBodyInfo = btRigidBody::btRigidBodyConstructionInfo;
using BulletCollisionShapePtr = std::unique_ptr<btCollisionShape>;

/// Stores information for constructing an `Ogre::RigidBody`.
/// This `Ogre::Resource` stores the collision shape and rigid body parameters
/// necessary to construct an `Ogre::RigidBody`, analogously to how an
/// `Ogre::Mesh` is used to construct an `Ogre::Entity`.
/// \attention `Ogre::CollisionShape` does not have a default loader, a manual
///            resource loader should be used.
class CollisionShape : public Ogre::Resource {
 public:
  CollisionShape(ResourceManager *creator,
                 const String &name,
                 ResourceHandle handle,
                 const String &group,
                 bool isManual = false,
                 ManualResourceLoader *loader = nullptr);

  ~CollisionShape() override {
    unload();
  }

  const RigidBodyInfo *getRigidBodyInfo() const noexcept;
  const btCollisionShape *getCollisionShape() const noexcept;

  void _setRigidBodyInfo(std::unique_ptr<RigidBodyInfo> info) noexcept;
  void _setCollisionShape(BulletCollisionShapePtr shape) noexcept;
  void _storeIndirectCollisionShapes(std::vector<BulletCollisionShapePtr> shapes) noexcept;
  void _setMeshInterface(std::unique_ptr<btStridingMeshInterface> mesh) noexcept;

  std::vector<uint16_t> &_getIndexBuffer() noexcept;
  std::vector<float> &_getVertexBuffer() noexcept;

 protected:
  void loadImpl() override;
  void unloadImpl() override;

 private:
  /// Owning pointer to the underlying rigid body construction info.
  std::unique_ptr<RigidBodyInfo> mInfo{};

  /// Owning pointer to the underlying collision shape. For performance reasons
  /// we don't want to duplicate the collision shape for multiple instances of
  /// the same `Ogre::RigidBody`.
  BulletCollisionShapePtr mCollisionShape{};

  /// Owning storage of child collision shapes, where necessary.
  /// `btCompoundShape` stores non-owning pointers to its children, but with
  /// only one `std::unique_ptr` we have no way of keeping track of them all.
  /// This vector stores the children so that they can be deleted properly.
  std::vector<BulletCollisionShapePtr> mIndirectShapes{};

  /// Index buffer for mesh-based collision shapes.
  /// Necessary because Bullet does not take ownership of the index data.
  std::vector<uint16_t> mIndexBuffer{};
  /// Vertex buffer for mesh-based collision shapes.
  /// Necessary because Bullet does not take ownership of the vertex data.
  std::vector<float> mVertexBuffer{};

  /// Interface to the `mIndexBuffer` and `mVertexBuffer`, needed for mesh-based
  /// collision hsapes.
  std::unique_ptr<btStridingMeshInterface> mMeshInterface{};
};

using CollisionShapePtr = std::shared_ptr<CollisionShape>;

} // namespace Ogre

#endif // OPENOBLIVION_OGREBULLET_COLLISION_SHAPE_HPP
