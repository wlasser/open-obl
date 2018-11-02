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

namespace character {

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
  float runModifier(float athleticsSkill) const noexcept {
    return *fMoveRunMult + *fMoveRunAthleticsMult * athleticsSkill * 0.01f;
  }

  // Multiplicative modifier of movement speed while swimming while 'walking'.
  float swimWalkModifier(float athleticsSkill) const noexcept {
    return *fMoveSwimWalkBase
        + *fMoveSwimWalkAthleticsMult * athleticsSkill * 0.01f;
  }

  // Multiplicative modifier of movement speed while swimming while 'running'.
  float swimRunModifier(float athleticsSkill) const noexcept {
    return *fMoveSwimRunBase
        + *fMoveSwimRunAthleticsMult * athleticsSkill * 0.01f;
  }

  // Multiplicative modifier of movement speed while sneaking.
  float sneakModifier() const noexcept {
    return *fMoveSneakMult;
  }

  // TODO: Incorporate into weapon state
  float encumbranceEffectModifier(bool hasWeaponOut) const noexcept {
    return hasWeaponOut ? *fMoveEncumEffect : *fMoveEncumEffectNoWea;
  }

  // Multiplicative modifier of movement speed due to items carried.
  float encumbranceModifier(float wornWeight,
                            bool hasWeaponOut) const noexcept {
    const float clampedWornWeight{std::min(wornWeight, *fMoveWeightMax)};
    const float weightRange{std::max(*fMoveWeightMax - *fMoveWeightMin, 0.1f)};
    const float effectMod{encumbranceEffectModifier(hasWeaponOut)};
    const float denominator{*fMoveWeightMin + clampedWornWeight};
    return 1.0f - effectMod * denominator / weightRange;
  }

  // Multiplicative modifier of movement speed due to having a weapon out.
  float weaponOutModifier(bool hasWeaponOut) const noexcept {
    return hasWeaponOut ? 1.0f : *fMoveNoWeaponMult;
  }

  // Base walk movement speed in units/s.
  float baseSpeed(float speedAttribute) const noexcept {
    const float walkRange{*fMoveCharWalkMax - *fMoveCharWalkMin};
    return *fMoveCharWalkMin + walkRange * speedAttribute * 0.01f;
  }

  // Overall movement speed while running, in m/s.
  float runSpeed(float speedAttribute,
                 float athleticsSkill,
                 float wornWeight,
                 float height,
                 bool hasWeaponOut) const noexcept {
    return baseSpeed(speedAttribute) * runModifier(athleticsSkill)
        * encumbranceModifier(wornWeight, hasWeaponOut) * height
        * conversions::metersPerUnit<float>;
  }

  // Overall movement speed while walking, in m/s.
  float walkSpeed(float speedAttribute,
                  float athleticsSkill,
                  float wornWeight,
                  float height,
                  bool hasWeaponOut) const noexcept {
    return baseSpeed(speedAttribute)
        * encumbranceModifier(wornWeight, hasWeaponOut)
        * height * conversions::metersPerUnit<float>;
  }

  // Overall movement speed while 'running' in water, in m/s.
  float swimRunSpeed(float speedAttribute,
                     float athleticsSkill,
                     float wornWeight,
                     float height,
                     bool hasWeaponOut) const noexcept {
    return baseSpeed(speedAttribute) * swimRunModifier(athleticsSkill)
        * encumbranceModifier(wornWeight, hasWeaponOut) * height
        * conversions::metersPerUnit<float>;
  }

  // Overall movement speed while 'walking' in water, in m/s.
  float swimWalkSpeed(float speedAttribute,
                      float athleticsSkill,
                      float wornWeight,
                      float height,
                      bool hasWeaponOut) const noexcept {
    return baseSpeed(speedAttribute) * swimWalkModifier(athleticsSkill)
        * encumbranceModifier(wornWeight, hasWeaponOut) * height
        * conversions::metersPerUnit<float>;
  }

  // Distance from jump apex to ground, in m.
  float jumpHeight(float acrobaticsSkill) const noexcept {
    const float heightRange{*fJumpHeightMax - *fJumpHeightMin};
    return (*fJumpHeightMin + heightRange * acrobaticsSkill * 0.01f)
        * conversions::metersPerUnit<float>;
  }

  float height{raceHeight * 128 * conversions::metersPerUnit < float > };
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

  float getMoveSpeed() const noexcept {
    const float base{baseSpeed(speedAttribute) * raceHeight
                         * conversions::metersPerUnit<float>};
    const float weightMult{encumbranceModifier(wornWeight, hasWeaponOut)};
    return base * weightMult
        * (speedModifier ? speedModifier(hasWeaponOut, isRunning) : 1.0f);
  }

  void reactivatePhysics() noexcept {
    rigidBody->activate(true);
  }

  void updateCameraOrientation() noexcept {
    cameraNode->setOrientation(Ogre::Quaternion(Ogre::Radian(0),
                                                Ogre::Vector3::UNIT_X));
    pitchNode->setOrientation(Ogre::Quaternion(Ogre::Radian(0),
                                               Ogre::Vector3::UNIT_X));
    pitchNode->pitch(pitch, Ogre::SceneNode::TS_LOCAL);
    cameraNode->yaw(yaw, Ogre::SceneNode::TS_LOCAL);
  }

  void move() noexcept {
    const auto speed{getMoveSpeed()};
    // This is a rotation of the standard basis, so is still in SO(3)
    const auto axes{cameraNode->getLocalAxes()};
    if (auto length = localVelocity.length() > 0.01f) {
      using namespace Ogre::conversions;
      const auto v{rigidBody->getLinearVelocity()};
      auto newV{toBullet(axes * localVelocity / length * speed)};
      newV.setY(v.y());
      rigidBody->setLinearVelocity(newV);
    } else {
      const auto v{rigidBody->getLinearVelocity()};
      rigidBody->setLinearVelocity({0.0f, v.y(), 0.0f});
    }
  }

  void updatePhysics(float /*elapsed*/) noexcept {
    reactivatePhysics();
    updateCameraOrientation();
    move();
  }

};

} // namespace character

#endif // OPENOBLIVION_CHARACTER_CONTROLLER_PLAYER_CONTROLLER_IMPL_HPP
