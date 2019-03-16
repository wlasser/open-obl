#ifndef OPENOBLIVION_OGREBULLET_RIGID_BODY_HPP
#define OPENOBLIVION_OGREBULLET_RIGID_BODY_HPP

#include "ogrebullet/collision_object.hpp"
#include "ogrebullet/motion_state.hpp"
#include "ogre/spdlog_listener.hpp"
#include <btBulletDynamicsCommon.h>
#include <OgreMovableObject.h>
#include <gsl/gsl>
#include <stdexcept>

namespace Ogre {

class RigidBody : public MovableObject, public MovableObject::Listener {
 public:
  /// \remark Passing `nullptr` means that the node was detached.
  void _notifyAttached(Node *parent, bool isTagPoint) override;

  void _notifyMoved() override;

  void _updateRenderQueue(RenderQueue *queue) override;

  const AxisAlignedBox &getBoundingBox() const override;

  Real getBoundingRadius() const override;

  const String &getMovableType() const override;

  void visitRenderables(Renderable::Visitor *visitor,
                        bool debugRenderables) override;

  ~RigidBody() override = default;

  btRigidBody *getRigidBody() const;

  /// Tell the physics system that the bound node has been transformed externally
  void notify();

  /// Scale the rigid body by scaling the collision shape. If an override is not
  /// already being used, this will create one and notify the rigid body of its
  /// new collision shape.
  /// \remark This operation should be avoided as much as possible, and ideally
  ///        called before the first physics update of the scene. If called
  ///        afterwards, there's no guarantee that you won't upset Bullet.
  void setScale(const Vector3 &scale);

  /// Disable or enable the Bullet `CF_NO_CONTACT_RESPONSE` flag.
  /// \remark If `enabled` is true then `CF_NO_CONTACT_RESPONSE` will *not* be
  ///         set.
  void setContactResponseEnabled(bool enabled);

  /// Return false if the Bullet `CF_NO_CONTACT_RESPONSE` flag is set, and true
  /// otherwise.
  bool getContactResponseEnabled() const noexcept;

 private:
  using flag_t = std::make_unsigned_t<
      std::underlying_type_t<btCollisionObject::CollisionFlags>>;

  friend class RigidBodyFactory;

  /// \remark `Ogre::RigidBody` objects should be created through
  ///         `Ogre::SceneManager::createMovableObject()`.
  explicit RigidBody(const String &name, CollisionObjectPtr collisionObject);

  CollisionObjectPtr mCollisionObject{};
  /// `btRigidBody` cannot be scaled; in order to scale on a per-instance basis,
  /// we use an override shape copied from the main collision shape and scaled.
  /// This does not need to be used if the scale is unity.
  std::unique_ptr<btCollisionShape> mCollisionShapeOverride{};
  std::unique_ptr<btRigidBody> mRigidBody{};
  std::unique_ptr<MotionState> mMotionState{};
  /// Needed because `getBoundingRadius()` is `const`, and demands to return by
  /// `const` reference.
  mutable AxisAlignedBox mBox{};
  const Ogre::String mType = "RigidBody";

  /// Binding to a `Ogre::Node` enables automatic synchronization of the
  /// `CollisionObject` position and orientation with the `Ogre::Node`'s
  /// position and orientation. Transforming a bound `Ogre::Node` directly
  /// should be avoided, and if necessary then `notify()` should be called.
  /// Calling `bind` a second time will release the previously bound node and,
  /// unless the new node is null, will bind to the new one.
  void bind(Node *node);
};

class RigidBodyFactory : public MovableObjectFactory {
 public:
  RigidBodyFactory() = default;
  ~RigidBodyFactory() override = default;

  void destroyInstance(gsl::owner<MovableObject *> obj) override;

  const String &getType() const override;

 protected:
  gsl::owner<MovableObject *>
  createInstanceImpl(const String &name,
                     const NameValuePairList *params) override;

  const String mType = "RigidBody";
};

/// This should only be used by `Ogre::RigidBodyFactory`, and is used to signify
/// during `Ogre::RigidBody` creation that the `Ogre::CollisionObject` specified
/// does not contain sufficient physics data to construct a `Ogre::RigidBody`.
/// \see `RigidBodyFactor::createInstanceImpl()`.
/// \todo Make this private and give the derived SceneManager friendship.
struct PartialCollisionObjectException : virtual std::runtime_error {
  explicit PartialCollisionObjectException(const std::string &what)
      : std::runtime_error(what) {}
};

} // namespace Ogre

#endif // OPENOBLIVION_OGRE_RIGID_BODY_HPP
