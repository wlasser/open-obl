#ifndef OPENOBLIVION_CHARACTER_CONTROLLER_PLAYER_CONTROLLER_IMPL_HPP
#define OPENOBLIVION_CHARACTER_CONTROLLER_PLAYER_CONTROLLER_IMPL_HPP

#include "conversions.hpp"
#include "game_settings.hpp"
#include "ogrebullet/conversions.hpp"
#include "ogrebullet/motion_state.hpp"
#include <btBulletDynamicsCommon.h>
#include <Ogre.h>
#include <memory>
#include <optional>

namespace oo {

struct PlayerControllerImpl {
  GameSetting<float> fMoveCharWalkMin{"fMoveCharWalkMin", 90.0f};
  GameSetting<float> fMoveCharWalkMax{"fMoveCharWalkMax", 130.0f};

  GameSetting<float> fMoveRunMult{"fMoveRunMult", 3.0f};
  GameSetting<float> fMoveRunAthleticsMult{"fMoveRunAthleticsMult", 1.0f};
  GameSetting<float> fMoveSwimWalkBase{"fMoveSwimWalkBase", 0.5f};
  GameSetting<float> fMoveSwimWalkAthleticsMult
      {"fMoveSwimWalkAthleticsMult", 0.02f};
  GameSetting<float> fMoveSwimRunBase{"fMoveSwimRunBase", 0.5f};
  GameSetting<float> fMoveSwimRunAthleticsMult
      {"fMoveSwimRunAthleticsMult", 0.1f};

  GameSetting<float> fJumpHeightMin{"fJumpHeightMin", 64.0f};
  GameSetting<float> fJumpHeightMax{"fJumpHeightMax", 164.0f};

  GameSetting<float> fMoveEncumEffect{"fMoveEncumEffect", 0.4f};
  GameSetting<float> fMoveEncumEffectNoWea{"fMoveEncumEffectNoWea", 0.3f};
  GameSetting<float> fMoveNoWeaponMult{"fMoveNoWeaponMult", 1.1f};
  GameSetting<float> fMoveWeightMin{"fMoveWeightMin", 0.0f};
  GameSetting<float> fMoveWeightMax{"fMoveWeightMax", 150.0f};

  GameSetting<float> fMoveSneakMult{"fMoveSneakMult", 0.6f};

  float speedAttribute{50.0f};
  float athleticsSkill{50.0f};
  float acrobaticsSkill{50.0f};
  float raceHeight{1.0f};
  float wornWeight{0.0f};

  bool hasWeaponOut{false};
  bool isRunning{false};
  // speedModifier(hasWeaponOut, isRunning) gives runModifier, swimWalkModifier,
  // or swimRunModifier, multiplied by fMoveNoWeaponMult if appropriate.
  std::function<float(bool, bool)> speedModifier{};

  // Multiplicative modifier of movement speed while running.
  float runModifier(float athleticsSkill) const noexcept;

  // Multiplicative modifier of movement speed while swimming while 'walking'.
  float swimWalkModifier(float athleticsSkill) const noexcept;

  // Multiplicative modifier of movement speed while swimming while 'running'.
  float swimRunModifier(float athleticsSkill) const noexcept;

  // Multiplicative modifier of movement speed while sneaking.
  float sneakModifier() const noexcept;

  // TODO: Incorporate into weapon state
  float encumbranceEffectModifier(bool hasWeaponOut) const noexcept;

  // Multiplicative modifier of movement speed due to items carried.
  float encumbranceModifier(float wornWeight, bool hasWeaponOut) const noexcept;

  // Multiplicative modifier of movement speed due to having a weapon out.
  float weaponOutModifier(bool hasWeaponOut) const noexcept;

  // Base walk movement speed in units/s.
  float baseSpeed(float speedAttribute) const noexcept;

  // Overall movement speed while running, in m/s.
  float runSpeed(float speedAttribute,
                 float athleticsSkill,
                 float wornWeight,
                 float height,
                 bool hasWeaponOut) const noexcept;

  // Overall movement speed while walking, in m/s.
  float walkSpeed(float speedAttribute,
                  float athleticsSkill,
                  float wornWeight,
                  float height,
                  bool hasWeaponOut) const noexcept;

  // Overall movement speed while 'running' in water, in m/s.
  float swimRunSpeed(float speedAttribute,
                     float athleticsSkill,
                     float wornWeight,
                     float height,
                     bool hasWeaponOut) const noexcept;

  // Overall movement speed while 'walking' in water, in m/s.
  float swimWalkSpeed(float speedAttribute,
                      float athleticsSkill,
                      float wornWeight,
                      float height,
                      bool hasWeaponOut) const noexcept;

  // Distance from jump apex to ground, in m.
  float jumpHeight(float acrobaticsSkill) const noexcept;

  float height{raceHeight * 128 * oo::metersPerUnit<float>};
  float mass{80.0f};

  Ogre::Radian pitch{0.0f};
  Ogre::Radian yaw{0.0f};
  Ogre::Vector3 localVelocity{Ogre::Vector3::ZERO};

  Ogre::SceneNode *cameraNode{};
  Ogre::SceneNode *pitchNode{};
  Ogre::Camera *camera{};

  Ogre::SceneNode *bodyNode{};
  std::unique_ptr<Ogre::MotionState> motionState{};
  std::unique_ptr<btCollisionShape> collisionShape{};
  std::unique_ptr<btRigidBody> rigidBody{};

  btDiscreteDynamicsWorld *world{};

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
