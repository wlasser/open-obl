#ifndef OPENOBLIVION_OGREBULLET_RIGID_BODY_HPP
#define OPENOBLIVION_OGREBULLET_RIGID_BODY_HPP

#include "ogrebullet/collision_object.hpp"
#include "ogrebullet/motion_state.hpp"
#include "ogre/spdlog_listener.hpp"
#include <btBulletDynamicsCommon.h>
#include <OgreMovableObject.h>
#include <stdexcept>

namespace Ogre {

class RigidBody : public MovableObject, public MovableObject::Listener {
 public:
  // Passing nullptr means that the node was detached
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

 private:
  friend class RigidBodyFactory;

  explicit RigidBody(const String &name, CollisionObjectPtr collisionObject);

  CollisionObjectPtr mCollisionObject{};
  // btRigidBody cannot be scaled. In order to scale on a per-instance basis,
  // we use an override shape copied from the main collision shape and scaled.
  // This does not need to be used if the scale is non-unity.
  std::unique_ptr<btCollisionShape> mCollisionShapeOverride{};
  std::unique_ptr<btRigidBody> mRigidBody{};
  std::unique_ptr<MotionState> mMotionState{};
  // Needed because getBoundingRadius is const, and demands to return by
  // const reference.
  mutable AxisAlignedBox mBox{};
  const Ogre::String mType = "RigidBody";

  // Binding to a SceneNode enables automatic synchronization of the
  // CollisionObject position and orientation with the SceneNode's position and
  // orientation. Transforming a bound SceneNode directly should be avoided, and
  // if necessary then notify should be called.
  // Calling bind a second time will release the previously bound node and,
  // unless the new node is null, will bind to the new one.
  void bind(Node *node);

  // Tell the physics system that the bound node has been transformed externally
  void notify();

  // Scale the rigid body by scaling the collision shape. If an override is not
  // already being used, this will create one and notify the rigid body of its
  // new collision shape.
  void setScale(const Vector3 &scale);
};

class RigidBodyFactory : public MovableObjectFactory {
 public:
  RigidBodyFactory() = default;

  ~RigidBodyFactory() override = default;

  void destroyInstance(MovableObject *obj) override;

  const String &getType() const override;

 protected:
  MovableObject *createInstanceImpl(const String &name,
                                    const NameValuePairList *params) override;

  const String mType = "RigidBody";
};

// This should only be used by RigidBodyFactory, and is used to signify during
// RigidBody creation that the CollisionObject specified does not contain
// sufficient physics data to construct a RigidBody.
// See RigidBodyFactor::createInstanceImpl.
// TODO: Make this private and give the derived SceneManager friendship.
struct PartialCollisionObjectException : virtual std::runtime_error {
  explicit PartialCollisionObjectException(const std::string &what)
      : std::runtime_error(what) {}
};

} // namespace Ogre

#endif // OPENOBLIVION_OGRE_RIGID_BODY_HPP
