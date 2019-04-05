#include "character_controller/movement.hpp"
#include "game_settings.hpp"
#include "math/conversions.hpp"

namespace oo {

float runModifier(float athleticsSkill) noexcept {
  static GameSetting<float> mult{"fMoveRunMult", 3.0f};
  static GameSetting<float> athleticsMult{"fMoveRunAthleticsMult", 1.0f};
  return *mult + *athleticsMult * athleticsSkill * 0.01f;
}

float swimWalkModifier(float athleticsSkill) noexcept {
  static GameSetting<float> base{"fMoveSwimWalkBase", 0.5f};
  static GameSetting<float> athleticsMult{"fMoveSwimWalkAthleticsMult", 0.02f};
  return *base + *athleticsMult * athleticsSkill * 0.01f;
}

float swimRunModifier(float athleticsSkill) noexcept {
  static GameSetting<float> base{"fMoveSwimRunBase", 0.5f};
  static GameSetting<float> athleticsMult{"fMoveSwimRunAthleticsMult", 0.1f};
  return *base + *athleticsMult * athleticsSkill * 0.01f;
}

float sneakModifier() noexcept {
  static GameSetting<float> mult{"fMoveSneakMult", 0.6f};
  return *mult;
}

float encumbranceEffectModifier(bool hasWeaponOut) noexcept {
  static GameSetting<float> effect{"fMoveEncumEffect", 0.4f};
  static GameSetting<float> effectNoWeap{"fMoveEncumEffectNoWea", 0.3f};
  return hasWeaponOut ? *effect : *effectNoWeap;
}

float encumbranceModifier(float wornWeight,
                          bool hasWeaponOut) noexcept {
  static GameSetting<float> minWeight{"fMoveWeightMin", 0.0f};
  static GameSetting<float> maxWeight{"fMoveWeightMax", 150.0f};
  const float clampedWornWeight{std::min(wornWeight, *maxWeight)};
  const float weightRange{std::max(*maxWeight - *minWeight, 0.1f)};
  const float effectMod{oo::encumbranceEffectModifier(hasWeaponOut)};
  const float denominator{*minWeight + clampedWornWeight};
  return 1.0f - effectMod * denominator / weightRange;
}

float weaponOutModifier(bool hasWeaponOut) noexcept {
  static GameSetting<float> noWeapMult{"fMoveNoWeaponMult", 1.1f};
  return hasWeaponOut ? 1.0f : *noWeapMult;
}

float baseSpeed(float speedAttribute) noexcept {
  static GameSetting<float> minWalk{"fMoveCharWalkMin", 90.0f};
  static GameSetting<float> maxWalk{"fMoveCharWalkMax", 130.0f};
  const float walkRange{*maxWalk - *minWalk};
  return *minWalk + walkRange * speedAttribute * 0.01f;
}

float runSpeed(float speedAttribute, float athleticsSkill, float wornWeight,
               float height, bool hasWeaponOut) noexcept {
  return oo::baseSpeed(speedAttribute) * oo::runModifier(athleticsSkill)
      * oo::encumbranceModifier(wornWeight, hasWeaponOut) * height
      * oo::metersPerUnit<float>;
}

float walkSpeed(float speedAttribute, float/*athleticsSkill*/, float wornWeight,
                float height, bool hasWeaponOut) noexcept {
  return oo::baseSpeed(speedAttribute)
      * oo::encumbranceModifier(wornWeight, hasWeaponOut)
      * height * oo::metersPerUnit<float>;
}

float swimRunSpeed(float speedAttribute, float athleticsSkill, float wornWeight,
                   float height, bool hasWeaponOut) noexcept {
  return oo::baseSpeed(speedAttribute) * oo::swimRunModifier(athleticsSkill)
      * oo::encumbranceModifier(wornWeight, hasWeaponOut) * height
      * oo::metersPerUnit<float>;
}

float swimWalkSpeed(float speedAttribute, float athleticsSkill,
                    float wornWeight, float height,
                    bool hasWeaponOut) noexcept {
  return oo::baseSpeed(speedAttribute) * oo::swimWalkModifier(athleticsSkill)
      * oo::encumbranceModifier(wornWeight, hasWeaponOut) * height
      * oo::metersPerUnit<float>;
}

float jumpHeight(float acrobaticsSkill) noexcept {
  GameSetting<float> minHeight{"fJumpHeightMin", 64.0f};
  GameSetting<float> maxHeight{"fJumpHeightMax", 164.0f};
  const float heightRange{*maxHeight - *minHeight};
  return (*minHeight + heightRange * acrobaticsSkill * 0.01f)
      * oo::metersPerUnit<float>;
}

} // namespace oo