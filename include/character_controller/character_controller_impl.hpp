#ifndef OPENOBL_CHARACTER_CONTROLLER_CHARACTER_CONTROLLER_IMPL_HPP
#define OPENOBL_CHARACTER_CONTROLLER_CHARACTER_CONTROLLER_IMPL_HPP

#include "game_settings.hpp"
#include "math/conversions.hpp"
#include "ogrebullet/conversions.hpp"
#include "ogrebullet/motion_state.hpp"
#include "record/actor_value.hpp"
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

  Ogre::SceneNode *mRootNode{};
  Ogre::SceneNode *mBodyNode{};
  std::unique_ptr<Ogre::MotionState> mMotionState{};
  std::unique_ptr<btCollisionShape> mCollisionShape{};
  std::unique_ptr<btRigidBody> mRigidBody{};

  float mSpeedAttribute{50.0f};
  float mAthleticsSkill{50.0f};
  float mAcrobaticsSkill{50.0f};
  float mRaceHeight{1.0f};
  float mWornWeight{0.0f};

  bool mHasWeaponOut{false};
  bool mIsRunning{false};
  // speedModifier(mHasWeaponOut, mIsRunning) gives runModifier, swimWalkModifier,
  // or swimRunModifier, multiplied by fMoveNoWeaponMult if appropriate.
  std::function<float(bool, bool)> mSpeedModifier{};

  float mHeight{mRaceHeight * 128 * oo::metersPerUnit<float>};
  float mMass{80.0f};

  Ogre::Radian mPitch{0.0f};
  Ogre::Radian mRootYaw{0.0f};
  Ogre::Radian mYaw{0.0f};
  Ogre::Vector3 mLocalVelocity{Ogre::Vector3::ZERO};

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

  gsl::not_null<const btRigidBody *> getRigidBody() const noexcept;
  gsl::not_null<btRigidBody *> getRigidBody() noexcept;

  gsl::not_null<const Ogre::SceneNode *> getCameraNode() const noexcept;
  gsl::not_null<Ogre::SceneNode *> getCameraNode() noexcept;

  gsl::not_null<const Ogre::SceneNode *> getRootNode() const noexcept;
  gsl::not_null<Ogre::SceneNode *> getRootNode() noexcept;

  Ogre::Radian getPitch() const noexcept;
  Ogre::Radian &getPitch() noexcept;

  Ogre::Radian getYaw() const noexcept;
  Ogre::Radian &getYaw() noexcept;

  Ogre::Radian getRootYaw() const noexcept;
  Ogre::Radian &getRootYaw() noexcept;

  Ogre::Vector3 &getLocalVelocity() noexcept;
  Ogre::Vector3 getLocalVelocity() const noexcept;

  float getSkill(oo::SkillIndex skill) const noexcept;
  float getMass() const noexcept;

  bool getIsRunning() const noexcept;
  void setIsRunning(bool isRunning) noexcept;

  float getHeight() const noexcept;

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

#endif // OPENOBL_CHARACTER_CONTROLLER_CHARACTER_CONTROLLER_IMPL_HPP
