#ifndef OPENOBLIVION_MOVEMENT_HPP
#define OPENOBLIVION_MOVEMENT_HPP

namespace oo {

/// Multiplicative modifier of movement speed while running.
float runModifier(float athleticsSkill) noexcept;

/// Multiplicative modifier of movement speed while swimming while 'walking'.
float swimWalkModifier(float athleticsSkill) noexcept;

/// Multiplicative modifier of movement speed while swimming while 'running'.
float swimRunModifier(float athleticsSkill) noexcept;

/// Multiplicative modifier of movement speed while sneaking.
float sneakModifier() noexcept;

// TODO: Incorporate into weapon state
float encumbranceEffectModifier(bool hasWeaponOut) noexcept;

/// Multiplicative modifier of movement speed due to items carried.
float encumbranceModifier(float wornWeight, bool hasWeaponOut) noexcept;

/// Multiplicative modifier of movement speed due to having a weapon out.
float weaponOutModifier(bool hasWeaponOut) noexcept;

/// Base walk movement speed in units/s.
float baseSpeed(float speedAttribute) noexcept;

/// Overall movement speed while running, in m/s.
float runSpeed(float speedAttribute,
               float athleticsSkill,
               float wornWeight,
               float height,
               bool hasWeaponOut) noexcept;

/// Overall movement speed while walking, in m/s.
float walkSpeed(float speedAttribute,
                float athleticsSkill,
                float wornWeight,
                float height,
                bool hasWeaponOut) noexcept;

/// Overall movement speed while 'running' in water, in m/s.
float swimRunSpeed(float speedAttribute,
                   float athleticsSkill,
                   float wornWeight,
                   float height,
                   bool hasWeaponOut) noexcept;

/// Overall movement speed while 'walking' in water, in m/s.
float swimWalkSpeed(float speedAttribute,
                    float athleticsSkill,
                    float wornWeight,
                    float height,
                    bool hasWeaponOut) noexcept;

/// Distance from jump apex to ground, in m.
float jumpHeight(float acrobaticsSkill) noexcept;

} // namespace oo

#endif //OPENOBLIVION_MOVEMENT_HPP
