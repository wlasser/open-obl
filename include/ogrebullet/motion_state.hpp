#ifndef OPENOBLIVION_OGREBULLET_MOTION_STATE_HPP
#define OPENOBLIVION_OGREBULLET_MOTION_STATE_HPP

#include <btBulletDynamicsCommon.h>
#include <Ogre.h>

namespace Ogre {

/// Synchronizes the position of a `Ogre::RigidBody` with an `Ogre::Node`.
/// By binding an `Ogre::MotionState` to an `Ogre::Node` and then pointing an
/// `Ogre::RigidBody` to the `Ogre::MotionState`, the position and orientation
/// of the `Ogre::Node` and `Ogre::RigidBody` are automatically kept in sync.
///
/// This class takes ownership over an `Ogre::Node` and all its children, in
/// the sense that no two `Ogre::MotionState` objects should point to the same
/// `Ogre::Node`, or to two different `Ogre::Node`s which share a parent that
/// is also pointed to by an `Ogre::MotionState`.
class MotionState : public btMotionState {
 public:
  explicit MotionState(Node *node);

  MotionState(const MotionState &) = delete;
  MotionState &operator=(const MotionState &) = delete;
  MotionState(MotionState &&) noexcept;
  MotionState &operator=(MotionState &&) noexcept;

  void getWorldTransform(btTransform &worldTrans) const override;
  void setWorldTransform(const btTransform &worldTrans) override;

  Vector3 getPosition() const noexcept {
    return mPosition;
  }

  Quaternion getOrientation() const noexcept {
    return mOrientation;
  }

  /// Synchronize the internal transform of the `Ogre::Node` with its external
  /// transform.
  /// One should avoid transforming the `Ogre::Node` explicitly, but if it must
  /// be done then call this function afterwards to resynchronize the stored
  /// transform to the `Ogre::Node`'s.
  /// \remark This function should be called whenever the `Ogre::Node` is
  ///         transformed externally at all, even indirectly by transforming
  ///         its parent.
  void notify();

 private:
  Node *mNode{};
  Vector3 mPosition{};
  Quaternion mOrientation{};
};

} // namespace Ogre

#endif // OPENOBLIVION_OGREBULLET_MOTION_STATE_HPP
