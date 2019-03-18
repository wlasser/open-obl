#ifndef OPENOBLIVION_CHARACTER_CONTROLLER_PLAYER_CONTROLLER_IMPL_HPP
#define OPENOBLIVION_CHARACTER_CONTROLLER_PLAYER_CONTROLLER_IMPL_HPP

#include "conversions.hpp"
#include "game_settings.hpp"
#include "ogrebullet/conversions.hpp"
#include "ogrebullet/motion_state.hpp"
#include <btBulletDynamicsCommon.h>
#include <gsl/gsl>
#include <Ogre.h>
#include <memory>
#include <optional>

namespace oo {

class PlayerControllerImpl {
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
  friend class PlayerController;

  explicit PlayerControllerImpl(gsl::not_null<Ogre::SceneManager *> scnMgr,
                                gsl::not_null<btDiscreteDynamicsWorld *> world);
  ~PlayerControllerImpl();
  PlayerControllerImpl(const PlayerControllerImpl &) = delete;
  PlayerControllerImpl &operator=(const PlayerControllerImpl &) = delete;
  /// \remark `mSpeedModifier` is left value-initialized, it is not moved from
  ///         `other`.
  PlayerControllerImpl(PlayerControllerImpl &&other) noexcept;
  /// \remark `mSpeedModifier` is left value-initialized, it is not moved from
  ///         `other`.
  PlayerControllerImpl &operator=(PlayerControllerImpl &&other) noexcept;

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

  float getSpringDisplacement() noexcept;
  float getMaxSpringDisplacement() noexcept;

  void applySpringForce(float displacement) noexcept;
  void updatePhysics(float /*elapsed*/) noexcept;
};

} // namespace oo

#endif // OPENOBLIVION_CHARACTER_CONTROLLER_PLAYER_CONTROLLER_IMPL_HPP
