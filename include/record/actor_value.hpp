#ifndef OPENOBL_ACTOR_VALUE_HPP
#define OPENOBL_ACTOR_VALUE_HPP

#include <cstdint>

namespace oo {

// Numbers specified for easy referencing
enum class ActorValue : uint32_t {
  Strength = 0,
  Intelligence = 1,
  Willpower = 2,
  Agility = 3,
  Speed = 4,
  Endurance = 5,
  Personality = 6,
  Luck = 7,
  Health = 8,
  Magicka = 9,
  Fatigue = 10,
  Encumbrance = 11,
  Armorer = 12,
  Athletics = 13,
  Blade = 14,
  Block = 15,
  Blunt = 16,
  HandToHand = 17,
  HeavyArmor = 18,
  Alchemy = 19,
  Alteration = 20,
  Conjuration = 21,
  Destruction = 22,
  Illusion = 23,
  Mysticism = 24,
  Restoration = 25,
  Acrobatics = 26,
  LightArmor = 27,
  Marksman = 28,
  Mercantile = 29,
  Security = 30,
  Sneak = 31,
  Speechcraft = 32,
  Aggression = 33,
  Confidence = 34,
  Energy = 35,
  Responsibility = 36,
  Bounty = 37,
  Fame = 38,
  Infamy = 39,
  MagickaMultiplier = 40,
  NightEyeBonus = 41,
  AttackBonus = 42,
  DefendBonus = 43,
  CastingPenalty = 44,
  Blindness = 45,
  Chameleon = 46,
  Invisibility = 47,
  Paralysis = 48,
  Silence = 49,
  Confusion = 50,
  DetectItemRange = 51,
  SpellAbsorbChance = 52,
  SpellReflectChance = 53,
  SwimSpeedMultiplier = 54,
  WaterBreathing = 55,
  WaterWalking = 56,
  StuntedMagicka = 57,
  DetectLifeRange = 58,
  ReflectDamage = 59,
  Telekinesis = 60,
  ResistFire = 61,
  ResistFrost = 62,
  ResistDisease = 63,
  ResistMagic = 64,
  ResistNormalWeapons = 65,
  ResistParalysis = 66,
  ResistPoison = 67,
  ResistShock = 68,
  Vampirism = 69,
  Darkness = 70,
  ResistWaterDamage = 71
};

enum class Specialization : uint32_t {
  Combat = 0,
  Magic = 1,
  Stealth = 2
};

// These are used sometimes instead of actor values. Or maybe actor values are
// used instead of these? They're in the same order just offset by -12.
enum class SkillIndex : int8_t {
  None = -1,
  Armorer = 0,
  Athletics = 1,
  Blade = 2,
  Block = 3,
  Blunt = 4,
  HandToHand = 5,
  HeavyArmor = 6,
  Alchemy = 7,
  Alteration = 8,
  Conjuration = 9,
  Destruction = 10,
  Illusion = 11,
  Mysticism = 12,
  Restoration = 13,
  Acrobatics = 14,
  LightArmor = 15,
  Marksman = 16,
  Mercantile = 17,
  Security = 18,
  Sneak = 19,
  Speechcraft = 20
};

} // namespace oo

#endif // OPENOBL_ACTOR_VALUE_HPP
