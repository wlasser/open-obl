#ifndef OPENOBLIVION_CHARACTER_CONTROLLER_CHARACTER_CONTROLLER_IMPL_HPP
#define OPENOBLIVION_CHARACTER_CONTROLLER_CHARACTER_CONTROLLER_IMPL_HPP

#include "game_settings.hpp"
#include "math/conversions.hpp"
#include "ogrebullet/conversions.hpp"
#include "ogrebullet/motion_state.hpp"
#include <btBulletDynamicsCommon.h>
#include <gsl/gsl>
#include <OgreMath.h>
#include <OgreCamera.h>
#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreVector.h>
#include <memory>
#include <optional>

namespace oo {

class CharacterControllerImpl {
 private:
  Ogre::SceneManager *mScnMgr;
  btDiscreteDynamicsWorld *mWorld;

  Ogre::SceneNode *mCameraNode{};
  Ogre::SceneNode *mPitchNode{};
  Ogre::Camera *mCamera{};

  Ogre::SceneNode *mBodyNode{};
  std::unique_ptr<Ogre::MotionState> mMotionState{};
  std::unique_ptr<btCollisionShape> mCollisionShape{};
  std::unique_ptr<btRigidBody> mRigidBody{};

  void attachCamera(gsl::not_null<Ogre::Camera *> camera,
                    gsl::not_null<Ogre::SceneNode *> node);
  void createAndAttachRigidBody(gsl::not_null<Ogre::SceneNode *> node);

 public:
  friend class CharacterController;

  explicit CharacterControllerImpl(gsl::not_null<Ogre::SceneManager *> scnMgr,
                                gsl::not_null<btDiscreteDynamicsWorld *> world);
  ~CharacterControllerImpl();
  CharacterControllerImpl(const CharacterControllerImpl &) = delete;
  CharacterControllerImpl &operator=(const CharacterControllerImpl &) = delete;
  /// \remark `mSpeedModifier` is left value-initialized, it is not moved from
  ///         `other`.
  CharacterControllerImpl(CharacterControllerImpl &&other) noexcept;
  /// \remark `mSpeedModifier` is left value-initialized, it is not moved from
  ///         `other`.
  CharacterControllerImpl &operator=(CharacterControllerImpl &&other) noexcept;

  float speedAttribute{50.0f};
  float athleticsSkill{50.0f};
  float acrobaticsSkill{50.0f};
  float raceHeight{1.0f};
  float wornWeight{0.0f};

  bool hasWeaponOut{false};
  bool isRunning{false};
  // speedModifier(hasWeaponOut, isRunning) gives runModifier, swimWalkModifier,
  // or swimRunModifier, multiplied by fMoveNoWeaponMult if appropriate.
  std::function<float(bool, bool)> mSpeedModifier{};

  float height{raceHeight * 128 * oo::metersPerUnit<float>};
  float mass{80.0f};

  Ogre::Radian pitch{0.0f};
  Ogre::Radian yaw{0.0f};
  Ogre::Vector3 localVelocity{Ogre::Vector3::ZERO};

  gsl::not_null<const btRigidBody *> getRigidBody() const noexcept;
  gsl::not_null<btRigidBody *> getRigidBody() noexcept;

  gsl::not_null<const Ogre::SceneNode *> getCameraNode() const noexcept;
  gsl::not_null<Ogre::SceneNode *> getCameraNode() noexcept;

  gsl::not_null<const Ogre::SceneNode *> getBodyNode() const noexcept;
  gsl::not_null<Ogre::SceneNode *> getBodyNode() noexcept;

  template<class F>
  void setSpeedModifier(F &&f) {
    mSpeedModifier = std::forward<F>(f);
  }

  float getMoveSpeed() const noexcept;

  float getCapsuleRadius() const noexcept;
  float getCapsuleHeight() const noexcept;

  void reactivatePhysics() noexcept;
  void updateCameraOrientation() noexcept;
  void move() noexcept;

  void setOrientation(Ogre::Radian pPitch, Ogre::Radian pYaw) noexcept;

  float getSpringDisplacement() noexcept;
  float getMaxSpringDisplacement() noexcept;

  void applySpringForce(float displacement) noexcept;
  void updatePhysics(float /*elapsed*/) noexcept;
};

} // namespace oo

#endif // OPENOBLIVION_CHARACTER_CONTROLLER_CHARACTER_CONTROLLER_IMPL_HPP
