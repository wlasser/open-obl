#ifndef OPENOBLIVION_SUBRECORDS_HPP
#define OPENOBLIVION_SUBRECORDS_HPP

#include "actor_value.hpp"
#include "attribute.hpp"
#include "formid.hpp"
#include "magic_effects.hpp"
#include "record/definition_helpers.hpp"
#include "record/io.hpp"
#include "record/record.hpp"
#include "record/subrecord.hpp"
#include "record/tuplifiable.hpp"
#include <array>
#include <istream>
#include <ostream>
#include <optional>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

namespace record {

// Records are either (top-level) records or subrecords, which are raw
// records with an appropriate wrapper.
namespace raw {

union Color {
  uint32_t v;
  struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t unused;
  };
};

// Sound to play for a door closing
using ANAM_DOOR = FormID;
// Apprentice skill text
using ANAM_SKIL = std::string;
// Loop sound for a door
using BNAM_DOOR = FormID;
// Crime gold multiplier for a faction
using CNAM_FACT = float;
// Default hair colour
using CNAM_RACE = uint8_t;
// ESM/ESP author. Max 512 bytes, for some reason
using CNAM_TES4 = std::string;
// Unfortunately there are different subrecords with the type DATA,
// and which one must be inferred from context.
using DATA_ALCH = float;
// Originally size of a master file, now unused
using DATA_TES4 = uint64_t;
// Class description
using DESC = std::string;
// Editor ID
using EDID = std::string;
// Magic effect ID
using EFID = EffectID;
// Expert skill text
using ENAM_SKIL = std::string;
// Facegen geometry (symmetric)
using FGGS = std::array<uint8_t, 200>;
// Facegen geometry (asymmetric)
using FGGA = std::array<uint8_t, 120>;
// Facegen texture (symmetric)
using FGTS = std::array<uint8_t, 200>;
// A floating point value, that doesn't necessarily represent a
// float. Unlike DATA_GMST, it's always stored as a float bit
// pattern, so the corresponding FNAM is meaningless (and harmful in
// the case of l).
using FLTV = float;
// Female faction rank name
using FNAM_FACT = std::string;
// Type of the next subrecord; s for short, l for long, f for float
// Used in GLOB records in conjunction with FLTV
using FNAM_GLOB = char;
// Light fade value
using FNAM_LIGH = float;
// Body data marker
using FNAM_RACE = std::tuple<>;
// Sound filename
using FNAM_SOUN = std::string;
// Name
using FULL = std::string;
// Possible grass on a landscape texture
using GNAM = FormID;
// Icon filename
using ICON = std::string;
// Faction rank insignia icon filename. Why not use ICON?
using INAM = std::string;
// Skill index for SKIL. Uses ActorValue not SkillIndex, for some reason.
using INDX_SKIL = ActorValue;
// Journeyman skill text
using JNAM_SKIL = std::string;
// ESM files used by the ESP, in load order
using MAST = std::string;
// Male faction rank name
using MNAM = std::string;
// Body data marker
using MNAM_RACE = std::tuple<>;
// Master skill text
using MNAM_SKIL = std::string;
// Bounding box radius
using MODB = float;
// Model filename
using MODL = std::string;
// Body data markers
using NAM0 = std::tuple<>;
using NAM1 = std::tuple<>;
// Base object formid
using NAME = FormID;
// Open by default. Its presence implies true.
using ONAM = std::tuple<>;
// Facegen main clamp
using PNAM = float;
// Rank index in a faction
using RNAM = uint32_t;
// Item script
using SCRI = FormID;
// ESM/ESP description. Also max 512 bytes
using SNAM = std::string;
// Sound to play for a door opening
using SNAM_DOOR = FormID;
// Sound to play for a light
using SNAM_LIGH = FormID;
// Landscape texture specular
using SNAM_LTEX = uint8_t;
// Body data, unused?
using SNAM_RACE = std::array<uint8_t, 2>;
// Door random teleport location
using TNAM_DOOR = FormID;
// Greater/lesser powers and racial abilities
using SPLO = FormID;
// Facegen face clamp
using UNAM = float;
// The climate of a cell, if it is exterior or an interior cell with the
// BehaveLikeExterior flag set.
using XCCM = FormID;
// Enchanted weapon charge
using XCHG = float;
// The water height in a cell
using XCLW = float;
// Number of copies of an item
using XCNT = int32_t;
// The water in a cell
using XCWT = FormID;
// If a cell is owned, disable ownership while this global variable is true.
using XGLB = FormID;
// Placed armor or weapon health. This is displayed and editable as a float
// in the construction set, but is only saved as an integer.
using XHLT = int32_t;
// Placed leveled creature level modifier
using XLCM = int32_t;
// Distant lod data
using XLOD = std::array<float, 3>;
// Denotes the reference as a marker?
using XMRK = std::tuple<>;
// Cell owner
using XOWN = FormID;
// Cell formid, only used in testing cells, associated to XMarkers.
using XPCI = FormID;
// If a cell is owned, and the owner is a faction, the rank in that faction.
using XRNK = uint32_t;
// Reference to a door with a random teleport target which can use this door as
// an output.
using XRTM = FormID;
// Uniform scaling factor for placed references
using XSCL = float;
// Target reference
using XTRG = FormID;

// Cell flags
enum class DATA_CELL : uint8_t {
  None = 0,
  CannotTravelFromHere = 1u,
  HasWater = 1u << 1u,
  OblivionOrForceHideLand = 1u << 3u,
  PublicPlace = 1u << 5u,
  HandChanged = 1u << 6u,
  BehaveLikeExterior = 1u << 7u
};
inline constexpr DATA_CELL operator|(DATA_CELL a, DATA_CELL b) {
  return DATA_CELL(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

// Eye flags
enum class DATA_EYES : uint8_t {
  None = 0,
  Playable = 1u
};
inline constexpr DATA_EYES operator|(DATA_EYES a, DATA_EYES b) {
  return DATA_EYES(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

// SpecialCombat means that faction members may fight each other without
// alerting other members of the faction.
enum class DATA_FACT : uint8_t {
  None = 0,
  InvisibleToPlayer = 1u,
  Evil = 1u << 1u,
  SpecialCombat = 1u << 2u
};
inline constexpr DATA_FACT operator|(DATA_FACT a, DATA_FACT b) {
  return DATA_FACT(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

// Hair flags
enum class DATA_HAIR : uint8_t {
  None = 0,
  Playable = 1u,
  NotMale = 1u << 1u, // Why the negation?
  NotFemale = 1u << 2u,
  Fixed = 1u << 3u
};
inline constexpr DATA_HAIR operator|(DATA_HAIR a, DATA_HAIR b) {
  return DATA_HAIR(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

// Door flags
enum class FNAM_DOOR : uint8_t {
  None = 0,
  OblivionGate = 1u,
  AutomaticDoor = 1u << 1u,
  Hidden = 1u << 2u,
  MinimalUse = 1u << 3u
};
inline constexpr FNAM_DOOR operator|(FNAM_DOOR a, FNAM_DOOR b) {
  return FNAM_DOOR(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

// Map marker flags
enum class FNAM_REFR : uint8_t {
  None = 0,
  Visible = 1u,
  CanTravelTo = 1u << 1u
};
inline constexpr FNAM_REFR operator|(FNAM_REFR a, FNAM_REFR b) {
  return FNAM_REFR(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

// Body part indices
enum class INDX_BODY : uint32_t {
  UpperBody = 0,
  LowerBody,
  Hand,
  Foot,
  Tail
};

// Facial part indices
enum class INDX_FACE : uint32_t {
  Head = 0,
  EarMale,
  EarFemale,
  Mouth,
  TeethLower,
  TeethUpper,
  Tongue,
  EyeLeft,
  EyeRight
};

// Map marker location type
enum class TNAM : uint16_t {
  None = 0,
  Camp = 1u,
  Cave = 2u,
  City = 3u,
  ElvenRuin = 4u,
  FortRuin = 5u,
  Mine = 6u,
  Landmark = 7u,
  Tavern = 8u,
  Settlement = 9u,
  DaedricShrine = 10u,
  OblivionGate = 11u,
  Door = 12u
};

// TODO: Find the remaining values of this
enum class XACT : uint32_t {
  None = 0,
  OpenByDefault = 0x0du
};

// Cell music type
enum class XCMT : uint8_t {
  Default = 0,
  Public = 1u,
  Dungeon = 2u
};

// Soul contained in soul gem
enum class XSOL : uint8_t {
  None = 0,
  Petty = 1u,
  Lesser = 2u,
  Common = 3u,
  Greater = 4u,
  Grand = 5u
};

// Starting attributes for a particular race
struct ATTR : Tuplifiable<std::array<uint8_t, 8>, std::array<uint8_t, 8>> {
  std::array<uint8_t, 8> male{};
  std::array<uint8_t, 8> female{};
  MAKE_AS_TUPLE(&male, &female)
};

// Class data. Skill the NPC trains (if applicable) is given as an actual
// skill index, but the major skills are given as actor values, for some reason.
struct DATA_CLAS {
  enum class Flag : uint32_t {
    None = 0u, // i.e. NPC only
    Playable = 1u,
    Guard = 2u
  };
  enum class BarterFlag : uint32_t {
    None = 0u,
    Weapons = 1u << 0u,
    Armor = 1u << 1u,
    Clothing = 1u << 2u,
    Books = 1u << 3u,
    Ingredients = 1u << 4u,
    Lights = 1u << 7u,
    Apparatus = 1u << 8u,
    Miscellaneous = 1u << 10u,
    Spells = 1u << 11u,
    MagicItems = 1u << 12u,
    Potions = 1u << 13u,
    Training = 1u << 14u,
    Recharge = 1u << 16u,
    Repair = 1u << 17u
  };
  std::array<ActorValue, 2> primaryAttributes{};
  Specialization specialization{};
  std::array<ActorValue, 7> majorSkills{};
  Flag playableFlag = Flag::None;
  BarterFlag barterFlag = BarterFlag::None;
  // If false the next three variables are not present
  bool hasTrainingInfo = false;
  SkillIndex skillTrained = SkillIndex::None;
  uint8_t maxTrainingLevel = 0u;
  std::array<uint8_t, 2> unused{};
};
inline constexpr DATA_CLAS::Flag operator|(DATA_CLAS::Flag a,
                                           DATA_CLAS::Flag b) {
  return DATA_CLAS::Flag(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
inline constexpr DATA_CLAS::BarterFlag operator|(DATA_CLAS::BarterFlag a,
                                                 DATA_CLAS::BarterFlag b) {
  return DATA_CLAS::BarterFlag(static_cast<uint32_t>(a)
                                   | static_cast<uint32_t>(b));
}

// Value of a game setting. Only one value is used at a time, but cannot use a
// union due to s.
// TODO: Use std::variant
struct DATA_GMST {
  float f = 0.0f;
  int32_t i = 0;
  std::vector<char> s{};
};

// Lighting data
struct DATA_LIGH {
  enum class Flag : uint32_t {
    None = 0u,
    Dynamic = 1u,
    CanBeCarried = 1u << 1u,
    Negative = 1u << 2u,
    Flicker = 1u << 3u,
    OffByDefault = 1u << 5u,
    FlickerSlow = 1u << 6u,
    Pulse = 1u << 7u,
    PulseSlow = 1u << 8u,
    SpotLight = 1u << 9u,
    SpotShadow = 1u << 10u
  };
  // Duration time in seconds for a carried light. -1 for no duration.
  int32_t time{};
  // Light radius in world units
  uint32_t radius{};
  // Light color
  Color color{};
  Flag flags = Flag::None;
  float falloffExponent{1.0f};
  // Spotlight field of view in degrees
  float fov{90.0f};
  // Item properties for carried lights
  uint32_t value{};
  float weight{};
};
inline constexpr DATA_LIGH::Flag operator|(DATA_LIGH::Flag a,
                                           DATA_LIGH::Flag b) {
  return DATA_LIGH::Flag(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
inline constexpr DATA_LIGH::Flag operator&(DATA_LIGH::Flag a,
                                           DATA_LIGH::Flag b) {
  return DATA_LIGH::Flag(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

struct DATA_MISC : Tuplifiable<int32_t, float> {
  int32_t value{};
  float weight{};
  MAKE_AS_TUPLE(&value, &weight);
};

struct DATA_MGEF {
  enum class Flag : uint32_t {
    None = 0u,
    Hostile = 1u,
    Recover = 1u << 1u,
    Detrimental = 1u << 2u,
    MagnitudePercent = 1u << 3u,
    Self = 1u << 4u,
    Touch = 1u << 5u,
    Target = 1u << 6u,
    NoDuration = 1u << 7u,
    NoMagnitude = 1u << 8u,
    NoArea = 1u << 9u,
    FXPersist = 1u << 10u,
    Spellmaking = 1u << 11u,
    Enchanting = 1u << 12u,
    NoIngredient = 1u << 13u,
    UseWeapon = 1u << 16u,
    UseArmor = 1u << 17u,
    UseCreature = 1u << 18u,
    UseSkill = 1u << 19u,
    SprayProjectile = 1u << 20u,
    BoltProjectile = 1u << 21u,
    FogProjectile = 1u << 22u,
    NoHitEffect = 1u << 23u
  };
  Flag flags = Flag::None;
  float baseCost = 0.0f;
  // For summon spells, the FormID of the summoned weapon, armor, or creature.
  // Otherwise, the ActorValue affected by the spell.
  union AssociatedObject {
    FormID summonedFormID;
    ActorValue affectedActorValue;
  };
  AssociatedObject associatedObject{};
  MagicSchool school{};
  // Resist magic always applies to Flag::Touch and Flag::Target, after this.
  // Seems to be 0xFFFFFFFF if no resist.
  ActorValue resist{};
  // The number of strings in the ESCE subrecord of the parent MGEF record
  uint16_t esceLength = 0;
  // Unknown. Probably unused, there is a lot of 0xCDCD which is used for
  // uninitialized heap memory on Windows, and often it is just 0.
  uint16_t unused = 0;
  // No light is saved as 0
  FormID light{};
  float projectileSpeed = 0.0f;
  FormID effectShader{};
  // The remaining values are present for every effect except Darkness, which
  // omits them all. Instead of making them optional, resort to the default
  // values if they are not present.
  FormID enchantEffect{};
  FormID castingSound{};
  FormID boltSound{};
  FormID hitSound{};
  FormID areaSound{};
  // Multiplies the magnitude of an enchantment
  float constantEffectEnchantmentFactor = 1.0f;
  // Multiplies the cost of an enchanted item
  float constantEffectBarterFactor = 1.0f;
};
inline constexpr DATA_MGEF::Flag operator|(DATA_MGEF::Flag a,
                                           DATA_MGEF::Flag b) {
  return static_cast<DATA_MGEF::Flag>(static_cast<uint32_t>(a)
      | static_cast<uint32_t>(b));
}

struct DATA_RACE {
  // The ActorValue is saved as a uint8_t, not a uint32_t like it usually is.
  // The CS enforces that there are seven skill modifiers, but allows any number
  // of them to be `NONE`. Such entries are not written to the record.
  std::vector<std::pair<ActorValue, int8_t>> skillModifiers{};
  // Padding?
  std::array<uint8_t, 2> unused{};
  float heightMale = 0.0f;
  float heightFemale = 0.0f;
  float weightMale = 0.0f;
  float weightFemale = 0.0f;
  enum class Flag : uint32_t {
    None = 0u,
    Playable = 1u
  };
  Flag flags = Flag::None;
};
inline constexpr DATA_RACE::Flag operator|(DATA_RACE::Flag a,
                                           DATA_RACE::Flag b) {
  return DATA_RACE::Flag(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

// Position and rotation data
struct DATA_REFR : Tuplifiable<float, float, float, float, float, float> {
  float x{};
  float y{};
  float z{};
  float aX{};
  float aY{};
  float aZ{};
  MAKE_AS_TUPLE(&x, &y, &z, &aX, &aY, &aZ);
};

struct DATA_SKIL : Tuplifiable<ActorValue,
                               Attribute,
                               Specialization,
                               std::pair<float, float>> {
  // Which skill trains this skill. Should agree with the index of the parent
  // SKIL record. Not a SkillIndex, for reasons.
  ActorValue index{};
  Attribute attribute{};
  Specialization specialization{};
  // Each skill has one or two actions associated to it, which when performed
  // grant experience points for that skill. Sometimes, if only one action is
  // possible then the first value is zero and the second is nonzero. Sometimes,
  // the first value applies and the second value is 1. Always read/write both.
  std::pair<float, float> experiencePerAction{};
  MAKE_AS_TUPLE(&index, &attribute, &specialization, &experiencePerAction)
};

// Unknown. Used to to mark deleted FormIDs, probably unused now
struct DELE {
  uint32_t size = 0;
};

// Default hair
struct DNAM : Tuplifiable<FormID, FormID> {
  FormID m{};
  FormID f{};
  MAKE_AS_TUPLE(&m, &f)
};

// Magic effect
struct EFIT {
  EffectID efid{};
  uint32_t magnitude = 0;
  uint32_t area = 0;
  uint32_t duration = 0;
  enum class Type : uint32_t {
    Self = 0u,
    Touch = 1u,
    Target = 2u
  };
  Type type = Type::Self;
  // Actor value index for attribute or skill effect
  ActorValue avIndex{};
};

// Eyes
struct ENAM {
  std::vector<FormID> eyes;
};

// Potion and ingredient value
struct ENIT {
  uint32_t value = 0;
  // Flag::NoAuto: Value is set manually and not calculated
  // Flag::Food: This is not an ingredient, it is food or drink
  enum class Flag : uint8_t {
    None = 0u,
    NoAuto = 1u << 0u,
    Food = 1u << 1u
  };
  Flag flags = Flag::None;
  // Unused data
  std::array<uint8_t, 3> unused{};
};
inline constexpr ENIT::Flag operator|(ENIT::Flag a, ENIT::Flag b) {
  return static_cast<ENIT::Flag>(static_cast<uint8_t>(a)
      | static_cast<uint8_t>(b));
}

// Enchanting effect
struct ENIT_ENCH {
  enum class Type : uint32_t {
    Scroll = 0u,
    Staff = 1u,
    Weapon = 2u,
    Apparel = 3u
  };
  Type type = Type::Scroll;
  uint32_t chargeAmount = 0;
  uint32_t chargeCost = 0;
  uint8_t noAutoCalculate = 0;
  std::array<uint8_t, 3> unused{};
};

// Counter effects for magic effects
struct ESCE {
  std::vector<EffectID> effects;
};

// ESM/ESP header
struct HEDR : Tuplifiable<float, int32_t, uint32_t> {
  float version = 0.8f;
  // Number of records and groups in the file, not including the TES4 record
  int32_t numRecords = 0;
  // Next available object ID?
  uint32_t nextObjectID = 0;
  MAKE_AS_TUPLE(&version, &numRecords, &nextObjectID)
};

// Hair
struct HNAM {
  std::vector<FormID> hair;
};

// Havok data for land materials
struct HNAM_LTEX {
  enum class MaterialType : uint8_t {
    Stone = 0,
    Cloth,
    Dirt,
    Glass,
    Grass,
    Metal,
    Organic,
    Skin,
    Water,
    Wood,
    HeavyStone,
    HeavyMetal,
    HeavyWood,
    Chain,
    Snow,
    StoneStairs,
    ClothStairs,
    DirtStairs,
    GlassStairs,
    GrassStairs,
    MetalStairs,
    OrganicStairs,
    SkinStairs,
    WaterStairs,
    WoodStairs,
    HeavyStoneStairs,
    HeavyMetalStairs,
    HeavyWoodStairs,
    ChainStairs,
    SnowStairs,
    Elevator
  };
  MaterialType type = MaterialType::Dirt;
  uint8_t friction = 30;
  uint8_t restitution = 30;
};

// Texture hashes
struct MODT {
  struct MODTRecord {
    uint64_t ddsHash = 0;
    uint64_t ddxHash = 0;
    uint64_t folderHash = 0;
  };
  std::vector<MODTRecord> records{};
};

// Some kind of unused offset(?) record. Format is partially known.
struct OFST {
  std::vector<std::array<uint32_t, 3>> unused{};
};

// Script effect
struct SCIT {
  FormID id{};
  MagicSchool school{};
  EffectID visualEffect{};
  // Flags
  enum class Flag : uint8_t {
    None = 0u,
    Hostile = 1u
  };
  Flag flags = Flag::None;
  // Unused
  std::array<uint8_t, 3> unused{};
};
inline constexpr SCIT::Flag operator|(SCIT::Flag a, SCIT::Flag b) {
  return static_cast<SCIT::Flag>(static_cast<uint8_t>(a)
      | static_cast<uint8_t>(b));
}

struct SNDD {
  // Multiply by 5 to convert to game units
  uint8_t minAttenuationDistance = 0;
  // Multiply by 100 to convert to game units
  uint8_t maxAttenuationDistance = 0;
  // As a signed percentage
  int8_t frequencyAdjustment = 0;
  // Unused? TODO: Find out what this does
  uint8_t unused = 0;
  // Flag::LFE: Low frequency effects
  enum class Flag : uint32_t {
    None = 0u,
    RandomFrequencyShift = 1u,
    PlayAtRandom = 1u << 1u,
    EnvironmentIgnored = 1u << 2u,
    RandomLocation = 1u << 3u,
    Loop = 1u << 4u,
    MenuSound = 1u << 5u,
    TwoDimensional = 1u << 6u,
    LFE = 1u << 7u
  };
  Flag flags = Flag::None;
};
inline constexpr SNDD::Flag operator|(SNDD::Flag a, SNDD::Flag b) {
  return static_cast<SNDD::Flag>(static_cast<uint32_t>(a)
      | static_cast<uint32_t>(b));
}

struct SNDX : SNDD {
  // Unused? TODO: Find out what this does
  uint32_t unusedWord = 0;
  // Divide by 100 to convert to dB
  std::optional<uint32_t> staticAttenuation = std::nullopt;
  // Multiply by 1440/256 to convert to minutes
  std::optional<uint8_t> startTime = std::nullopt;
  // Multiply by 1440/256 to convert to minutes
  std::optional<uint8_t> stopTime = std::nullopt;
};

// Base attributes for races
// Spell data
struct SPIT {
  enum class Type : uint32_t {
    Spell = 0,
    Disease = 1,
    Power = 2,
    LesserPower = 3,
    Ability = 4,
    Poison = 5
  };
  enum class Level : uint32_t {
    Novice = 0,
    Apprentice = 1,
    Journeyman = 2,
    Expert = 3,
    Master = 4
  };
  enum class Flag : uint32_t {
    None = 0,
    NoAuto = 0x1u,
    NoSilence = 0x8u | 0x2u,
    PlayerStartSpell = 0x4u,
    AreaIgnoresLineOfSight = 0x10u,
    ScriptAlwaysApplies = 0x20u,
    NoAbsorbReflect = 0x40u,
    TouchExplodeNoTarget = 0x80u
  };
  Type type{Type::Spell};
  uint32_t cost{};
  Level level{Level::Novice};
  Flag flags{Flag::None};
};

// Race determining voice
struct VNAM : Tuplifiable<FormID, FormID> {
  FormID m{};
  FormID f{};
  MAKE_AS_TUPLE(&m, &f)
};

// Coordinates of an exterior cell
struct XCLC : Tuplifiable<uint32_t, uint32_t> {
  uint32_t x = 0;
  uint32_t y = 0;
  MAKE_AS_TUPLE(&x, &y)
};

// Interior cell lighting
struct XCLL : Tuplifiable<Color, Color, Color, float, float, uint32_t, uint32_t,
                          float, float> {
  Color ambient{};
  Color directional{};
  Color fogColor{};
  float fogNear = 0.0f;
  float fogFar = 0.0f;
  uint32_t rotationXY = 0;
  uint32_t rotationZ = 0;
  float directionalFade = 0.0f;
  float fogClipDist = 0.0f;
  MAKE_AS_TUPLE(&ambient, &directional, &fogColor, &fogNear, &fogFar,
                &rotationXY, &rotationZ, &directionalFade, &fogClipDist)
};

// The regions containing the cell
struct XCLR {
  std::vector<FormID> regions;
};

struct XESP {
  enum class Flag : uint32_t {
    None = 0,
    SetEnableStateToOppositeOfParent = 1u
  };
  FormID parent{};
  Flag flags{Flag::None};
};
inline constexpr XESP::Flag operator|(XESP::Flag a, XESP::Flag b) {
  return static_cast<XESP::Flag>(static_cast<uint32_t>(a)
      | static_cast<uint32_t>(b));
}

// Locked door/container information
struct XLOC {
  enum class Flag : uint32_t {
    None = 0,
    LeveledLock = 4u
  };
  // 0-100, 100 = needs a key
  uint32_t lockLevel{};
  // 0 if no key
  FormID key{};
  // Unknown four bytes sometimes present
  uint32_t unused{};
  Flag flags = Flag::None;
};
inline constexpr XLOC::Flag operator|(XLOC::Flag a, XLOC::Flag b) {
  return XLOC::Flag(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

// Disposition modifier between members of different factions
// Also used for racial relations
struct XNAM : Tuplifiable<FormID, int32_t> {
  FormID factionID{};
  int32_t relationModifier = 0;
  MAKE_AS_TUPLE(&factionID, &relationModifier)
};

// Ragdoll data
struct XRGD {
  std::vector<uint8_t> bytes{};
};

// Speed tree information TODO: Support trees
struct XSED {
  uint16_t size{};
};

// Teleport information for a door
struct XTEL : Tuplifiable<FormID, float, float, float, float, float, float> {
  FormID destinationID{};
  // Destination position
  float x{};
  float y{};
  float z{};
  // Destination angle
  float aX{};
  float aY{};
  float aZ{};
  MAKE_AS_TUPLE(&destinationID, &x, &y, &z, &aX, &aY, &aZ);
};

} // namespace raw

// Wrapped subrecords
using ATTR = Subrecord<raw::ATTR, "ATTR"_rec>;
using DELE = Subrecord<raw::DELE, "DELE"_rec>;
using DESC = Subrecord<raw::DESC, "DESC"_rec>;
using DNAM = Subrecord<raw::DNAM, "DNAM"_rec>;
using EDID = Subrecord<raw::EDID, "EDID"_rec>;
using EFID = Subrecord<raw::EFID, "EFID"_rec>;
using EFIT = Subrecord<raw::EFIT, "EFIT"_rec>;
using ENAM = Subrecord<raw::ENAM, "ENAM"_rec>;
using ENIT = Subrecord<raw::ENIT, "ENIT"_rec>;
using ESCE = Subrecord<raw::ESCE, "ESCE"_rec>;
using FGGA = Subrecord<raw::FGGA, "FGGA"_rec>;
using FGGS = Subrecord<raw::FGGS, "FGGS"_rec>;
using FGTS = Subrecord<raw::FGTS, "FGTS"_rec>;
using FLTV = Subrecord<raw::FLTV, "FLTV"_rec>;
using FULL = Subrecord<raw::FULL, "FULL"_rec>;
using GNAM = Subrecord<raw::GNAM, "GNAM"_rec>;
using HEDR = Subrecord<raw::HEDR, "HEDR"_rec>;
using HNAM = Subrecord<raw::HNAM, "HNAM"_rec>;
using ICON = Subrecord<raw::ICON, "ICON"_rec>;
using INAM = Subrecord<raw::INAM, "INAM"_rec>;
using MAST = Subrecord<raw::MAST, "MAST"_rec>;
using MNAM = Subrecord<raw::MNAM, "MNAM"_rec>;
using MODB = Subrecord<raw::MODB, "MODB"_rec>;
using MODL = Subrecord<raw::MODL, "MODL"_rec>;
using MODT = Subrecord<raw::MODT, "MODT"_rec>;
using NAM0 = Subrecord<raw::NAM0, "NAM0"_rec>;
using NAM1 = Subrecord<raw::NAM1, "NAM1"_rec>;
using NAME = Subrecord<raw::NAME, "NAME"_rec>;
using OFST = Subrecord<raw::OFST, "OFST"_rec>;
using ONAM = Subrecord<raw::ONAM, "ONAM"_rec>;
using PNAM = Subrecord<raw::PNAM, "PNAM"_rec>;
using RNAM = Subrecord<raw::RNAM, "RNAM"_rec>;
using SCIT = Subrecord<raw::SCIT, "SCIT"_rec>;
using SCRI = Subrecord<raw::SCRI, "SCRI"_rec>;
using SNAM = Subrecord<raw::SNAM, "SNAM"_rec>;
using SNDD = Subrecord<raw::SNDD, "SNDD"_rec>;
using SNDX = Subrecord<raw::SNDX, "SNDX"_rec>;
using SPIT = Subrecord<raw::SPIT, "SPIT"_rec>;
using SPLO = Subrecord<raw::SPLO, "SPLO"_rec>;
using TNAM = Subrecord<raw::TNAM, "TNAM"_rec>;
using UNAM = Subrecord<raw::UNAM, "UNAM"_rec>;
using VNAM = Subrecord<raw::VNAM, "VNAM"_rec>;
using XACT = Subrecord<raw::XACT, "XACT"_rec>;
using XCCM = Subrecord<raw::XCCM, "XCCM"_rec>;
using XCHG = Subrecord<raw::XCHG, "XCHG"_rec>;
using XCLC = Subrecord<raw::XCLC, "XCLC"_rec>;
using XCLL = Subrecord<raw::XCLL, "XCLL"_rec>;
using XCLR = Subrecord<raw::XCLR, "XCLR"_rec>;
using XCLW = Subrecord<raw::XCLW, "XCLW"_rec>;
using XCMT = Subrecord<raw::XCMT, "XCMT"_rec>;
using XCNT = Subrecord<raw::XCNT, "XCNT"_rec>;
using XCWT = Subrecord<raw::XCWT, "XCWT"_rec>;
using XESP = Subrecord<raw::XESP, "XESP"_rec>;
using XGLB = Subrecord<raw::XGLB, "XGLB"_rec>;
using XHLT = Subrecord<raw::XHLT, "XHLT"_rec>;
using XLCM = Subrecord<raw::XLCM, "XLCM"_rec>;
using XLOC = Subrecord<raw::XLOC, "XLOC"_rec>;
using XLOD = Subrecord<raw::XLOD, "XLOD"_rec>;
using XMRK = Subrecord<raw::XMRK, "XMRK"_rec>;
using XNAM = Subrecord<raw::XNAM, "XNAM"_rec>;
using XPCI = Subrecord<raw::XPCI, "XPCI"_rec>;
using XOWN = Subrecord<raw::XOWN, "XOWN"_rec>;
using XRGD = Subrecord<raw::XRGD, "XRGD"_rec>;
using XRNK = Subrecord<raw::XRNK, "XRNK"_rec>;
using XRTM = Subrecord<raw::XRTM, "XRTM"_rec>;
using XSCL = Subrecord<raw::XSCL, "XSCL"_rec>;
using XSED = Subrecord<raw::XSED, "XSED"_rec>;
using XSOL = Subrecord<raw::XSOL, "XSOL"_rec>;
using XTEL = Subrecord<raw::XTEL, "XTEL"_rec>;
using XTRG = Subrecord<raw::XTRG, "XTRG"_rec>;

using ANAM_DOOR = Subrecord<raw::ANAM_DOOR, "ANAM"_rec>;
using ANAM_SKIL = Subrecord<raw::ANAM_SKIL, "ANAM"_rec>;
using BNAM_DOOR = Subrecord<raw::BNAM_DOOR, "BNAM"_rec>;
using CNAM_FACT = Subrecord<raw::CNAM_FACT, "CNAM"_rec>;
using CNAM_RACE = Subrecord<raw::CNAM_RACE, "CNAM"_rec>;
using CNAM_TES4 = Subrecord<raw::CNAM_TES4, "CNAM"_rec>;
using DATA_ALCH = Subrecord<raw::DATA_ALCH, "DATA"_rec>;
using DATA_CELL = Subrecord<raw::DATA_CELL, "DATA"_rec>;
using DATA_CLAS = Subrecord<raw::DATA_CLAS, "DATA"_rec>;
using DATA_EYES = Subrecord<raw::DATA_EYES, "DATA"_rec>;
using DATA_FACT = Subrecord<raw::DATA_FACT, "DATA"_rec>;
using DATA_GMST = Subrecord<raw::DATA_GMST, "DATA"_rec>;
using DATA_HAIR = Subrecord<raw::DATA_HAIR, "DATA"_rec>;
using DATA_LIGH = Subrecord<raw::DATA_LIGH, "DATA"_rec>;
using DATA_MISC = Subrecord<raw::DATA_MISC, "DATA"_rec>;
using DATA_MGEF = Subrecord<raw::DATA_MGEF, "DATA"_rec>;
using DATA_RACE = Subrecord<raw::DATA_RACE, "DATA"_rec>;
using DATA_REFR = Subrecord<raw::DATA_REFR, "DATA"_rec>;
using DATA_SKIL = Subrecord<raw::DATA_SKIL, "DATA"_rec>;
using DATA_TES4 = Subrecord<raw::DATA_TES4, "DATA"_rec>;
using ENAM_SKIL = Subrecord<raw::ENAM_SKIL, "ENAM"_rec>;
using ENIT_ENCH = Subrecord<raw::ENIT_ENCH, "ENIT"_rec>;
using FNAM_DOOR = Subrecord<raw::FNAM_DOOR, "FNAM"_rec>;
using FNAM_FACT = Subrecord<raw::FNAM_FACT, "FNAM"_rec>;
using FNAM_GLOB = Subrecord<raw::FNAM_GLOB, "FNAM"_rec>;
using FNAM_LIGH = Subrecord<raw::FNAM_LIGH, "FNAM"_rec>;
using FNAM_RACE = Subrecord<raw::FNAM_RACE, "FNAM"_rec>;
using FNAM_REFR = Subrecord<raw::FNAM_REFR, "FNAM"_rec>;
using FNAM_SOUN = Subrecord<raw::FNAM_SOUN, "FNAM"_rec>;
using HNAM_LTEX = Subrecord<raw::HNAM_LTEX, "HNAM"_rec>;
using INDX_BODY = Subrecord<raw::INDX_BODY, "INDX"_rec>;
using INDX_FACE = Subrecord<raw::INDX_FACE, "INDX"_rec>;
using INDX_SKIL = Subrecord<raw::INDX_SKIL, "INDX"_rec>;
using JNAM_SKIL = Subrecord<raw::JNAM_SKIL, "JNAM"_rec>;
using MNAM_RACE = Subrecord<raw::MNAM_RACE, "MNAM"_rec>;
using MNAM_SKIL = Subrecord<raw::MNAM_SKIL, "MNAM"_rec>;
using SNAM_DOOR = Subrecord<raw::SNAM_DOOR, "SNAM"_rec>;
using SNAM_LIGH = Subrecord<raw::SNAM_LIGH, "SNAM"_rec>;
using SNAM_LTEX = Subrecord<raw::SNAM_LTEX, "SNAM"_rec>;
using SNAM_RACE = Subrecord<raw::SNAM_RACE, "SNAM"_rec>;
using TNAM_DOOR = Subrecord<raw::TNAM_DOOR, "TNAM"_rec>;

DECLARE_SPECIALIZED_SUBRECORD(DATA_CLAS);
DECLARE_SPECIALIZED_SUBRECORD(DATA_GMST);
DECLARE_SPECIALIZED_SUBRECORD(DATA_LIGH);
DECLARE_SPECIALIZED_SUBRECORD(DATA_MGEF);
DECLARE_SPECIALIZED_SUBRECORD(DATA_RACE);
DECLARE_SPECIALIZED_SUBRECORD(DELE);
DECLARE_SPECIALIZED_SUBRECORD(EFID);
DECLARE_SPECIALIZED_SUBRECORD(ENAM);
DECLARE_SPECIALIZED_SUBRECORD(EFIT);
DECLARE_SPECIALIZED_SUBRECORD(ENIT);
DECLARE_SPECIALIZED_SUBRECORD(ENIT_ENCH);
DECLARE_SPECIALIZED_SUBRECORD(ESCE);
DECLARE_SPECIALIZED_SUBRECORD(HNAM);
DECLARE_SPECIALIZED_SUBRECORD(HNAM_LTEX);
DECLARE_SPECIALIZED_SUBRECORD(MODT);
DECLARE_SPECIALIZED_SUBRECORD(OFST);
DECLARE_SPECIALIZED_SUBRECORD(SCIT);
DECLARE_SPECIALIZED_SUBRECORD(SNDD);
DECLARE_SPECIALIZED_SUBRECORD(SNDX);
DECLARE_SPECIALIZED_SUBRECORD(SPIT);
DECLARE_SPECIALIZED_SUBRECORD(XCLR);
DECLARE_SPECIALIZED_SUBRECORD(XESP);
DECLARE_SPECIALIZED_SUBRECORD(XLOC);
DECLARE_SPECIALIZED_SUBRECORD(XRGD);
DECLARE_SPECIALIZED_SUBRECORD(XSED);

} // namespace record

#endif // OPENOBLIVION_SUBRECORDS_HPP
