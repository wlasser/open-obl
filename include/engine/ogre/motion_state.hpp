#ifndef OPENOBLIVION_ENGINE_MOTION_STATE_HPP
#define OPENOBLIVION_ENGINE_MOTION_STATE_HPP

#include <btBulletDynamicsCommon.h>
#include <Ogre.h>

namespace Ogre {

// By binding a MotionState to a SceneNode and then pointing a RigidBody to the
// MotionState, the position and orientation of the SceneNode and RigidBody are
// automatically kept in sync.
// This class takes ownership over a SceneNode and all its children, in the
// sense that no two MotionState objects should point to the same SceneNode, or
// to two different SceneNodes which share a parent that is also pointed to by
// a MotionState.
class MotionState : public btMotionState {
 public:
  explicit MotionState(Node *node);

  MotionState(const MotionState &) = delete;
  MotionState &operator=(const MotionState &) = delete;
  MotionState(MotionState &&) noexcept;
  MotionState &operator=(MotionState &&) noexcept;

  void getWorldTransform(btTransform &worldTrans) const override;
  void setWorldTransform(const btTransform &worldTrans) override;

  Vector3 getPosition() const {
    return mPosition;
  }

  Quaternion getOrientation() const {
    return mOrientation;
  }

  // One should avoid transforming the SceneNode explicitly, but if it must be
  // done then call this function afterwards to resync the stored transform to
  // the SceneNode's.
  void notify();

 private:
  Node *mNode{};
  Vector3 mPosition{};
  Quaternion mOrientation{};
};

} // namespace Ogre

#endif // OPENOBLIVION_ENGINE_MOTION_STATE_HPP
