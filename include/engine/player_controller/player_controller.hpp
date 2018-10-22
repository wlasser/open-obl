#ifndef OPENOBLIVION_ENGINE_PLAYER_CONTROLLER_HPP
#define OPENOBLIVION_ENGINE_PLAYER_CONTROLLER_HPP

#include "engine/controls.hpp"
#include "engine/conversions.hpp"
#include "engine/player_controller/abilities.hpp"
#include "engine/player_controller/player_jump_state.hpp"
#include "engine/player_controller/player_stand_state.hpp"
#include "engine/player_controller/player_state.hpp"
#include "ogrebullet/motion_state.hpp"
#include "game_settings.hpp"
#include <btBulletDynamicsCommon.h>
#include <Ogre.h>
#include <array>
#include <variant>

namespace engine {

class PlayerController {
 public:
  explicit PlayerController(Ogre::SceneManager *scnMgr);

  Ogre::Camera *getCamera() const noexcept;
  btRigidBody *getRigidBody() const noexcept;

  void handleEvent(const KeyVariant &event);
  void handleEvent(const MouseVariant &event);

  void update(float elapsed);

  void moveTo(const Ogre::Vector3 &position);

  void handleCollision(const btCollisionObject *other,
                       const btManifoldPoint &contact);

 private:
  friend PlayerState;
  friend class PlayerStandState;
  friend class PlayerJumpState;
  friend struct MoveAbility;
  friend struct LookAbility;
  std::shared_ptr<PlayerState> state{};

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
  float encumbranceMultiplier(float wornWeight,
                              bool hasWeaponOut) const noexcept {
    const float clampedWornWeight = std::min(wornWeight, *fMoveWeightMax);
    const float weightRange = std::max(*fMoveWeightMax - *fMoveWeightMin, 0.1f);
    const float effectMod = encumbranceEffectModifier(hasWeaponOut);
    return 1.0f - effectMod * (*fMoveWeightMin + wornWeight) / weightRange;
  }

  // Base walk movement speed in units/s.
  float baseSpeed(float speedAttribute) const noexcept {
    const float walkRange = *fMoveCharWalkMax - *fMoveCharWalkMin;
    return *fMoveCharWalkMin + walkRange * speedAttribute * 0.01f;
  }

  // Overall movement speed while running, in m/s.
  float runSpeed(float speedAttribute,
                 float athleticsSkill,
                 float wornWeight,
                 float height,
                 bool hasWeaponOut) const noexcept {
    return baseSpeed(speedAttribute) * runModifier(athleticsSkill)
        * encumbranceMultiplier(wornWeight, hasWeaponOut) * height
        * conversions::metersPerUnit<float>;
  }

  // Overall movement speed while walking, in m/s.
  float walkSpeed(float speedAttribute,
                  float athleticsSkill,
                  float wornWeight,
                  float height,
                  bool hasWeaponOut) const noexcept {
    return baseSpeed(speedAttribute)
        * encumbranceMultiplier(wornWeight, hasWeaponOut)
        * height * conversions::metersPerUnit<float>;
  }

  // Overall movement speed while 'running' in water, in m/s.
  float swimRunSpeed(float speedAttribute,
                     float athleticsSkill,
                     float wornWeight,
                     float height,
                     bool hasWeaponOut) const noexcept {
    return baseSpeed(speedAttribute) * swimRunModifier(athleticsSkill)
        * encumbranceMultiplier(wornWeight, hasWeaponOut) * height
        * conversions::metersPerUnit<float>;
  }

  // Overall movement speed while 'walking' in water, in m/s.
  float swimWalkSpeed(float speedAttribute,
                      float athleticsSkill,
                      float wornWeight,
                      float height,
                      bool hasWeaponOut) const noexcept {
    return baseSpeed(speedAttribute) * swimWalkModifier(athleticsSkill)
        * encumbranceMultiplier(wornWeight, hasWeaponOut) * height
        * conversions::metersPerUnit<float>;
  }

  // Distance from jump apex to ground, in m.
  float jumpHeight(float acrobaticsSkill) const noexcept {
    const float heightRange = *fJumpHeightMax - *fJumpHeightMin;
    return (*fJumpHeightMin + heightRange * acrobaticsSkill * 0.01f)
        * conversions::metersPerUnit<float>;
  }

  void updatePhysics(float elapsed);

  float height{raceHeight * 128 * conversions::metersPerUnit<float>};
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
};

} // namespace engine

#endif // OPENOBLIVION_ENGINE_PLAYER_CONTROLLER_HPP
