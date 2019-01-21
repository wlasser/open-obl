#ifndef OPENOBLIVION_SUBRECORDS_HPP
#define OPENOBLIVION_SUBRECORDS_HPP

#include "bitflag.hpp"
#include "record/actor_value.hpp"
#include "record/attribute.hpp"
#include "record/definition_helpers.hpp"
#include "record/formid.hpp"
#include "record/magic_effects.hpp"
#include "record/subrecord.hpp"
#include "record/tuplifiable.hpp"
#include <array>
#include <iosfwd>
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
using ANAM_DOOR = oo::BaseId;
// Apprentice skill text
using ANAM_SKIL = std::string;
// Loop sound for a door
using BNAM_DOOR = oo::BaseId;
// Crime gold multiplier for a faction
using CNAM_FACT = float;
// NPC class
using CNAM_NPC_ = oo::BaseId;
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
// Editor Id
using EDID = std::string;
// Magic effect Id
using EFID = oo::EffectId;
// NPC eyes
using ENAM_NPC_ = oo::BaseId;
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
// Unknown face data
using FNAM_NPC_ = uint16_t;
// Body data marker
using FNAM_RACE = std::tuple<>;
// Sound filename
using FNAM_SOUN = std::string;
// Name
using FULL = std::string;
// Possible grass on a landscape texture
using GNAM = oo::BaseId;
// NPC hair colour
using HCLR = Color;
// NPC hair
using HNAM_NPC_ = oo::BaseId;
// Icon filename
using ICON = std::string;
// Faction rank insignia icon filename. Why not use ICON?
using INAM = std::string;
// FormId of item to drop on death.
using INAM_NPC_ = oo::BaseId;
// Skill index for SKIL. Uses oo::ActorValue not oo::SkillIndex, for some reason.
using INDX_SKIL = oo::ActorValue;
// Journeyman skill text
using JNAM_SKIL = std::string;
// NPC hair length
using LNAM = float;
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
using NAME = oo::BaseId;
// Open by default. Its presence implies true.
using ONAM = std::tuple<>;
// AI package formid
using PKID = oo::BaseId;
// Facegen main clamp
using PNAM = float;
// Rank index in a faction
using RNAM = uint32_t;
// NPC race
using RNAM_NPC_ = oo::BaseId;
// Item script
using SCRI = oo::BaseId;
// ESM/ESP description. Also max 512 bytes
using SNAM = std::string;
// Sound to play for an activator
using SNAM_ACTI = oo::BaseId;
// Sound to play for a door opening
using SNAM_DOOR = oo::BaseId;
// Sound to play for a light
using SNAM_LIGH = oo::BaseId;
// Landscape texture specular
using SNAM_LTEX = uint8_t;
// Body data, unused?
using SNAM_RACE = std::array<uint8_t, 2>;
// Door random teleport location. Either a CELL or WRLD.
using TNAM_DOOR = oo::BaseId;
// Greater/lesser powers, racial abilities, and spells.
using SPLO = oo::BaseId;
// Facegen face clamp
using UNAM = float;
// The climate of a cell, if it is exterior or an interior cell with the
// BehaveLikeExterior flag set.
using XCCM = oo::BaseId;
// Enchanted weapon charge
using XCHG = float;
// The water height in a cell
using XCLW = float;
// Number of copies of an item
using XCNT = int32_t;
// The water in a cell
using XCWT = oo::BaseId;
// If a cell is owned, disable ownership while this global variable is true.
using XGLB = oo::BaseId;
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
using XOWN = oo::BaseId;
// Cell formid, only used in testing cells, associated to XMarkers.
using XPCI = oo::BaseId;
// If a cell is owned, and the owner is a faction, the rank in that faction.
using XRNK = uint32_t;
// Reference to a door with a random teleport target which can use this door as
// an output.
using XRTM = oo::RefId;
// Uniform scaling factor for placed references
using XSCL = float;
// Target reference
using XTRG = oo::RefId;
// NPC combat style
using ZNAM = oo::BaseId;

// Cell flags
struct DATA_CELL : Bitflag<8, DATA_CELL> {
  static constexpr enum_t None{0u};
  static constexpr enum_t CannotTravelFromHere{1u};
  static constexpr enum_t HasWater{1u << 1u};
  static constexpr enum_t OblivionOrForceHideLand{1u << 3u};
  static constexpr enum_t PublicPlace{1u << 5u};
  static constexpr enum_t HandChanged{1u << 6u};
  static constexpr enum_t BehaveLikeExterior{1u << 7u};
};

// Eye flags
struct DATA_EYES : Bitflag<8, DATA_EYES> {
  static constexpr enum_t None{0u};
  static constexpr enum_t Playable{1u};
};

// SpecialCombat means that faction members may fight each other without
// alerting other members of the faction.
struct DATA_FACT : Bitflag<8, DATA_FACT> {
  static constexpr enum_t None{0};
  static constexpr enum_t InvisibleToPlayer{1u};
  static constexpr enum_t Evil{1u << 1u};
  static constexpr enum_t SpecialCombat{1u << 2u};
};

// Hair flags
struct DATA_HAIR : Bitflag<8, DATA_HAIR> {
  static constexpr enum_t None{0};
  static constexpr enum_t Playable{1u};
  static constexpr enum_t NotMale{1u << 1u}; // Why the negation?
  static constexpr enum_t NotFemale{1u << 2u};
  static constexpr enum_t Fixed{1u << 3u};
};

// Door flags
struct FNAM_DOOR : Bitflag<8, FNAM_DOOR> {
  static constexpr enum_t None{0};
  static constexpr enum_t OblivionGate{1u};
  static constexpr enum_t AutomaticDoor{1u << 1u};
  static constexpr enum_t Hidden{1u << 2u};
  static constexpr enum_t MinimalUse{1u << 3u};
};

// Map marker flags
struct FNAM_REFR : Bitflag<8, FNAM_REFR> {
  static constexpr enum_t None{0};
  static constexpr enum_t Visible{1u};
  static constexpr enum_t CanTravelTo{1u << 1u};
};

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
struct XACT : Bitflag<32, XACT> {
  static constexpr enum_t None{0};
  static constexpr enum_t OpenByDefault{0x0du};
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

// NPC base settings
struct ACBS {
  struct Flag : Bitflag<32, Flag> {
    static constexpr enum_t None{0};
    static constexpr enum_t Female{1u << 0u};
    static constexpr enum_t Essential{1u << 1u};
    static constexpr enum_t Respawn{1u << 3u};
    // Implied by PCLevelOffset, even though the enum is not set up that way.
    static constexpr enum_t AutoCalculate{1u << 4u};
    // If true calcMin and calcMax give the minimum and maximum level that the
    // NPC can be after applying the level offset.
    static constexpr enum_t PCLevelOffset{1u << 7u};
    static constexpr enum_t NoLowLevelProcessing{1u << 9u};
    static constexpr enum_t NoRumors{1u << 13u};
    static constexpr enum_t Summonable{1u << 14u};
    static constexpr enum_t NoPersuasion{1u << 15u};
    static constexpr enum_t CanCorpseCheck{1u << 20u};
  };
  Flag flags{Flag::make(Flag::None)};
  uint16_t baseSpellPoints{};
  uint16_t baseFatigue{};
  uint16_t barterGold{};
  // Offset to the player's level, when PCLevelOffset is set.
  int16_t level{};
  // Minimum value to clamp generated level to when AutoCalculate is set.
  uint16_t calcMin{};
  // Maximum value to clamp generated level to when AutoCalculate is set.
  uint16_t calcMax{};
};

// NPC AI data
struct AIDT {
  struct Flag : Bitflag<32, Flag> {
    static constexpr enum_t None{0};
    static constexpr enum_t Weapons{1u << 0u};
    static constexpr enum_t Armor{1u << 1u};
    static constexpr enum_t Clothing{1u << 2u};
    static constexpr enum_t Books{1u << 3u};
    static constexpr enum_t Ingredients{1u << 4u};
    static constexpr enum_t Lights{1u << 7u};
    static constexpr enum_t Apparatus{1u << 8u};
    static constexpr enum_t Miscellaneous{1u << 10u};
    static constexpr enum_t Spells{1u << 11u};
    static constexpr enum_t MagicItems{1u << 12u};
    static constexpr enum_t Potions{1u << 13u};
    static constexpr enum_t Training{1u << 14u};
    static constexpr enum_t Recharge{1u << 16u};
    static constexpr enum_t Repair{1u << 17u};
  };

  uint8_t aggression{};
  uint8_t confidence{};
  uint8_t energyLevel{};
  uint8_t responsibility{};
  Flag flags{Flag::make(Flag::None)};
  oo::SkillIndex trainingSkill{};
  uint8_t trainingLevel{};
  uint16_t unknown{};
};

// Starting attributes for a particular race
struct ATTR : Tuplifiable<std::array<uint8_t, 8>, std::array<uint8_t, 8>> {
  std::array<uint8_t, 8> male{};
  std::array<uint8_t, 8> female{};
  MAKE_AS_TUPLE(&male, &female)
};

// Item in a container
struct CNTO : Tuplifiable<oo::BaseId, uint32_t> {
  oo::BaseId id{};
  uint32_t count{};
  MAKE_AS_TUPLE(&id, &count);
};

// Class data. Skill the NPC trains (if applicable) is given as an actual
// skill index, but the major skills are given as actor values, for some reason.
struct DATA_CLAS {
  struct Flag : Bitflag<32, Flag> {
    static constexpr enum_t None{0}; // i.e. NPC only
    static constexpr enum_t Playable{1u};
    static constexpr enum_t Guard{2u};
  };
  struct BarterFlag : Bitflag<32, BarterFlag> {
    static constexpr enum_t None{0u};
    static constexpr enum_t Weapons{1u << 0u};
    static constexpr enum_t Armor{1u << 1u};
    static constexpr enum_t Clothing{1u << 2u};
    static constexpr enum_t Books{1u << 3u};
    static constexpr enum_t Ingredients{1u << 4u};
    static constexpr enum_t Lights{1u << 7u};
    static constexpr enum_t Apparatus{1u << 8u};
    static constexpr enum_t Miscellaneous{1u << 10u};
    static constexpr enum_t Spells{1u << 11u};
    static constexpr enum_t MagicItems{1u << 12u};
    static constexpr enum_t Potions{1u << 13u};
    static constexpr enum_t Training{1u << 14u};
    static constexpr enum_t Recharge{1u << 16u};
    static constexpr enum_t Repair{1u << 17u};
  };
  std::array<oo::ActorValue, 2> primaryAttributes{};
  oo::Specialization specialization{};
  std::array<oo::ActorValue, 7> majorSkills{};
  Flag playableFlag{Flag::make(Flag::None)};
  BarterFlag barterFlag{BarterFlag::make(BarterFlag::None)};
  // If false the next three variables are not present
  bool hasTrainingInfo = false;
  oo::SkillIndex skillTrained = oo::SkillIndex::None;
  uint8_t maxTrainingLevel = 0u;
  std::array<uint8_t, 2> unused{};
};

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
  struct Flag : Bitflag<32, Flag> {
    static constexpr enum_t None{0u};
    static constexpr enum_t Dynamic{1u};
    static constexpr enum_t CanBeCarried{1u << 1u};
    static constexpr enum_t Negative{1u << 2u};
    static constexpr enum_t Flicker{1u << 3u};
    static constexpr enum_t OffByDefault{1u << 5u};
    static constexpr enum_t FlickerSlow{1u << 6u};
    static constexpr enum_t Pulse{1u << 7u};
    static constexpr enum_t PulseSlow{1u << 8u};
    static constexpr enum_t SpotLight{1u << 9u};
    static constexpr enum_t SpotShadow{1u << 10u};
  };
  // Duration time in seconds for a carried light. -1 for no duration.
  int32_t time{};
  // Light radius in world units
  uint32_t radius{};
  // Light color
  Color color{};
  Flag flags{Flag::make(Flag::None)};
  float falloffExponent{1.0f};
  // Spotlight field of view in degrees
  float fov{90.0f};
  // Item properties for carried lights
  uint32_t value{};
  float weight{};
};

struct DATA_MISC : Tuplifiable<int32_t, float> {
  int32_t value{};
  float weight{};
  MAKE_AS_TUPLE(&value, &weight);
};

struct DATA_MGEF {
  struct Flag : Bitflag<32, Flag> {
    static constexpr enum_t None{0u};
    static constexpr enum_t Hostile{1u};
    static constexpr enum_t Recover{1u << 1u};
    static constexpr enum_t Detrimental{1u << 2u};
    static constexpr enum_t MagnitudePercent{1u << 3u};
    static constexpr enum_t Self{1u << 4u};
    static constexpr enum_t Touch{1u << 5u};
    static constexpr enum_t Target{1u << 6u};
    static constexpr enum_t NoDuration{1u << 7u};
    static constexpr enum_t NoMagnitude{1u << 8u};
    static constexpr enum_t NoArea{1u << 9u};
    static constexpr enum_t FXPersist{1u << 10u};
    static constexpr enum_t Spellmaking{1u << 11u};
    static constexpr enum_t Enchanting{1u << 12u};
    static constexpr enum_t NoIngredient{1u << 13u};
    static constexpr enum_t UseWeapon{1u << 16u};
    static constexpr enum_t UseArmor{1u << 17u};
    static constexpr enum_t UseCreature{1u << 18u};
    static constexpr enum_t UseSkill{1u << 19u};
    static constexpr enum_t SprayProjectile{1u << 20u};
    static constexpr enum_t BoltProjectile{1u << 21u};
    static constexpr enum_t FogProjectile{1u << 22u};
    static constexpr enum_t NoHitEffect{1u << 23u};
  };
  // For summon spells, the oo::BaseId of the summoned weapon, armor, or
  // creature. Otherwise, the oo::ActorValue affected by the spell.
  // TODO: std::variant
  using AssociatedObject = std::variant<oo::BaseId /*summonedBaseId*/,
                                        oo::ActorValue /*affectedActorValue*/>;
  Flag flags{Flag::make(Flag::None)};
  float baseCost = 0.0f;
  AssociatedObject associatedObject{};
  oo::MagicSchool school{};
  // Resist magic always applies to Flag::Touch and Flag::Target, after this.
  // Seems to be 0xFFFFFFFF if no resist.
  oo::ActorValue resist{};
  // The number of strings in the ESCE subrecord of the parent MGEF record
  uint16_t esceLength = 0;
  // Unknown. Probably unused, there is a lot of 0xCDCD which is used for
  // uninitialized heap memory on Windows, and often it is just 0.
  uint16_t unused = 0;
  // No light is saved as 0
  oo::BaseId light{};
  float projectileSpeed = 0.0f;
  oo::BaseId effectShader{};
  // The remaining values are present for every effect except Darkness, which
  // omits them all. Instead of making them optional, resort to the default
  // values if they are not present.
  oo::BaseId enchantEffect{};
  oo::BaseId castingSound{};
  oo::BaseId boltSound{};
  oo::BaseId hitSound{};
  oo::BaseId areaSound{};
  // Multiplies the magnitude of an enchantment
  float constantEffectEnchantmentFactor = 1.0f;
  // Multiplies the cost of an enchanted item
  float constantEffectBarterFactor = 1.0f;
};

struct DATA_NPC_ : Tuplifiable<std::array<uint8_t, 21>,
                               uint32_t,
                               std::array<uint8_t, 8>> {
  // Indexed by oo::SkillIndex
  std::array<uint8_t, 21> skills{};
  uint32_t health{};
  // Indexed by oo::Attribute
  std::array<uint8_t, 8> attributes{};
  MAKE_AS_TUPLE(&skills, &health, &attributes);
};

struct DATA_RACE {
  struct Flag : Bitflag<32, Flag> {
    static constexpr enum_t None{0u};
    static constexpr enum_t Playable{1u};
  };
  // The oo::ActorValue is saved as a uint8_t, not a uint32_t like usual.
  // The CS enforces that there are seven skill modifiers, but allows any number
  // of them to be `NONE`. Such entries are not written to the record.
  std::vector<std::pair<oo::ActorValue, int8_t>> skillModifiers{};
  // Padding?
  std::array<uint8_t, 2> unused{};
  float heightMale = 0.0f;
  float heightFemale = 0.0f;
  float weightMale = 0.0f;
  float weightFemale = 0.0f;
  Flag flags{Flag::make(Flag::None)};
};

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

struct DATA_SKIL : Tuplifiable<oo::ActorValue,
                               oo::Attribute,
                               oo::Specialization,
                               std::pair<float, float>> {
  // Which skill trains this skill. Should agree with the index of the parent
  // SKIL record. Not a oo::SkillIndex, for reasons.
  oo::ActorValue index{};
  oo::Attribute attribute{};
  oo::Specialization specialization{};
  // Each skill has one or two actions associated to it, which when performed
  // grant experience points for that skill. Sometimes, if only one action is
  // possible then the first value is zero and the second is nonzero. Sometimes,
  // the first value applies and the second value is 1. Always read/write both.
  std::pair<float, float> experiencePerAction{};
  MAKE_AS_TUPLE(&index, &attribute, &specialization, &experiencePerAction)
};

// Unknown. Used to to mark deleted FormIds, probably unused now
struct DELE {
  uint32_t size = 0;
};

// Default hair
struct DNAM : Tuplifiable<oo::BaseId, oo::BaseId> {
  oo::BaseId m{};
  oo::BaseId f{};
  MAKE_AS_TUPLE(&m, &f)
};

// Magic effect
struct EFIT {
  enum class Type : uint32_t {
    Self = 0u,
    Touch = 1u,
    Target = 2u
  };
  oo::EffectId efid{};
  uint32_t magnitude = 0;
  uint32_t area = 0;
  uint32_t duration = 0;
  Type type = Type::Self;
  // Actor value index for attribute or skill effect
  oo::ActorValue avIndex{};
};

// Eyes
struct ENAM {
  std::vector<oo::BaseId> eyes;
};

// Potion and ingredient value
struct ENIT {
  struct Flag : Bitflag<8, Flag> {
    static constexpr enum_t None{0u};
    // Value is set manually and not calculated
    static constexpr enum_t NoAuto{1u << 0u};
    // This is not an ingredient, it is food or drink
    static constexpr enum_t Food{1u << 1u};
  };
  uint32_t value = 0;
  Flag flags{Flag::make(Flag::None)};
  // Unused data
  std::array<uint8_t, 3> unused{};
};

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
  std::vector<oo::EffectId> effects;
};

// ESM/ESP header
struct HEDR : Tuplifiable<float, int32_t, uint32_t> {
  float version = 0.8f;
  // Number of records and groups in the file, not including the TES4 record
  int32_t numRecords = 0;
  // Next available object Id?
  uint32_t nextObjectId = 0;
  MAKE_AS_TUPLE(&version, &numRecords, &nextObjectId)
};

// Hair
struct HNAM {
  std::vector<oo::BaseId> hair;
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
  struct Flag : Bitflag<8, Flag> {
    static constexpr enum_t None{0u};
    static constexpr enum_t Hostile{1u};
  };
  oo::BaseId id{};
  oo::MagicSchool school{};
  oo::EffectId visualEffect{};
  Flag flags{Flag::make(Flag::None)};
  // Unused
  std::array<uint8_t, 3> unused{};
};

// NPC faction membership information
struct SNAM_NPC_ : Tuplifiable<oo::BaseId, uint8_t, std::array<uint8_t, 3>> {
  oo::BaseId factionId{};
  uint8_t rank{};
  std::array<uint8_t, 3> unused{};
  MAKE_AS_TUPLE(&factionId, &rank, &unused);
};

struct SNDD {
  struct Flag : Bitflag<32, Flag> {
    static constexpr enum_t None{0u};
    static constexpr enum_t RandomFrequencyShift{1u};
    static constexpr enum_t PlayAtRandom{1u << 1u};
    static constexpr enum_t EnvironmentIgnored{1u << 2u};
    static constexpr enum_t RandomLocation{1u << 3u};
    static constexpr enum_t Loop{1u << 4u};
    static constexpr enum_t MenuSound{1u << 5u};
    static constexpr enum_t TwoDimensional{1u << 6u};
    // Low frequency effects
    static constexpr enum_t LFE{1u << 7u};
  };
  // Multiply by 5 to convert to game units
  uint8_t minAttenuationDistance = 0;
  // Multiply by 100 to convert to game units
  uint8_t maxAttenuationDistance = 0;
  // As a signed percentage
  int8_t frequencyAdjustment = 0;
  // Unused? TODO: Find out what this does
  uint8_t unused = 0;
  Flag flags{Flag::make(Flag::None)};
};

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
  struct Flag : Bitflag<32, Flag> {
    static constexpr enum_t None{0};
    static constexpr enum_t NoAuto{0x1u};
    static constexpr enum_t NoSilence{0x8u | 0x2u};
    static constexpr enum_t PlayerStartSpell{0x4u};
    static constexpr enum_t AreaIgnoresLineOfSight{0x10u};
    static constexpr enum_t ScriptAlwaysApplies{0x20u};
    static constexpr enum_t NoAbsorbReflect{0x40u};
    static constexpr enum_t TouchExplodeNoTarget{0x80u};
  };
  Type type{Type::Spell};
  uint32_t cost{};
  Level level{Level::Novice};
  Flag flags{Flag::make(Flag::None)};
};

// Race determining voice
struct VNAM : Tuplifiable<oo::BaseId, oo::BaseId> {
  oo::BaseId m{};
  oo::BaseId f{};
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
  std::vector<oo::BaseId> regions;
};

struct XESP {
  struct Flag : Bitflag<32, Flag> {
    static constexpr enum_t None{0};
    static constexpr enum_t SetEnableStateToOppositeOfParent{1u};
  };
  oo::RefId parent{};
  Flag flags{Flag::make(Flag::None)};
};

// Locked door/container information
struct XLOC {
  struct Flag : Bitflag<32, Flag> {
    static constexpr enum_t None{0};
    static constexpr enum_t LeveledLock{4u};
  };
  // 0-100, 100 = needs a key
  uint32_t lockLevel{};
  // 0 if no key
  oo::BaseId key{};
  // Unknown four bytes sometimes present
  uint32_t unused{};
  Flag flags{Flag::make(Flag::None)};
};

// Disposition modifier between members of different factions
// Also used for racial relations
struct XNAM : Tuplifiable<oo::BaseId, int32_t> {
  oo::BaseId factionId{};
  int32_t relationModifier = 0;
  MAKE_AS_TUPLE(&factionId, &relationModifier)
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
struct XTEL : Tuplifiable<oo::RefId, float, float, float, float, float, float> {
  oo::RefId destinationId{};
  // Destination position
  float x{};
  float y{};
  float z{};
  // Destination angle
  float aX{};
  float aY{};
  float aZ{};
  MAKE_AS_TUPLE(&destinationId, &x, &y, &z, &aX, &aY, &aZ);
};

} // namespace raw

// Wrapped subrecords
using ACBS = Subrecord<raw::ACBS, "ACBS"_rec>;
using AIDT = Subrecord<raw::AIDT, "AIDT"_rec>;
using ATTR = Subrecord<raw::ATTR, "ATTR"_rec>;
using CNTO = Subrecord<raw::CNTO, "CNTO"_rec>;
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
using HCLR = Subrecord<raw::HCLR, "HCLR"_rec>;
using HEDR = Subrecord<raw::HEDR, "HEDR"_rec>;
using HNAM = Subrecord<raw::HNAM, "HNAM"_rec>;
using ICON = Subrecord<raw::ICON, "ICON"_rec>;
using INAM = Subrecord<raw::INAM, "INAM"_rec>;
using LNAM = Subrecord<raw::LNAM, "LNAM"_rec>;
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
using PKID = Subrecord<raw::PKID, "PKID"_rec>;
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
using ZNAM = Subrecord<raw::ZNAM, "ZNAM"_rec>;

using ANAM_DOOR = Subrecord<raw::ANAM_DOOR, "ANAM"_rec>;
using ANAM_SKIL = Subrecord<raw::ANAM_SKIL, "ANAM"_rec>;
using BNAM_DOOR = Subrecord<raw::BNAM_DOOR, "BNAM"_rec>;
using CNAM_FACT = Subrecord<raw::CNAM_FACT, "CNAM"_rec>;
using CNAM_NPC_ = Subrecord<raw::CNAM_NPC_, "CNAM"_rec>;
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
using DATA_NPC_ = Subrecord<raw::DATA_NPC_, "DATA"_rec>;
using DATA_RACE = Subrecord<raw::DATA_RACE, "DATA"_rec>;
using DATA_REFR = Subrecord<raw::DATA_REFR, "DATA"_rec>;
using DATA_SKIL = Subrecord<raw::DATA_SKIL, "DATA"_rec>;
using DATA_TES4 = Subrecord<raw::DATA_TES4, "DATA"_rec>;
using ENAM_NPC_ = Subrecord<raw::ENAM_NPC_, "ENAM"_rec>;
using ENAM_SKIL = Subrecord<raw::ENAM_SKIL, "ENAM"_rec>;
using ENIT_ENCH = Subrecord<raw::ENIT_ENCH, "ENIT"_rec>;
using FNAM_DOOR = Subrecord<raw::FNAM_DOOR, "FNAM"_rec>;
using FNAM_FACT = Subrecord<raw::FNAM_FACT, "FNAM"_rec>;
using FNAM_GLOB = Subrecord<raw::FNAM_GLOB, "FNAM"_rec>;
using FNAM_LIGH = Subrecord<raw::FNAM_LIGH, "FNAM"_rec>;
using FNAM_NPC_ = Subrecord<raw::FNAM_NPC_, "FNAM"_rec>;
using FNAM_RACE = Subrecord<raw::FNAM_RACE, "FNAM"_rec>;
using FNAM_REFR = Subrecord<raw::FNAM_REFR, "FNAM"_rec>;
using FNAM_SOUN = Subrecord<raw::FNAM_SOUN, "FNAM"_rec>;
using HNAM_LTEX = Subrecord<raw::HNAM_LTEX, "HNAM"_rec>;
using HNAM_NPC_ = Subrecord<raw::HNAM_NPC_, "HNAM"_rec>;
using INAM_NPC_ = Subrecord<raw::INAM_NPC_, "INAM"_rec>;
using INDX_BODY = Subrecord<raw::INDX_BODY, "INDX"_rec>;
using INDX_FACE = Subrecord<raw::INDX_FACE, "INDX"_rec>;
using INDX_SKIL = Subrecord<raw::INDX_SKIL, "INDX"_rec>;
using JNAM_SKIL = Subrecord<raw::JNAM_SKIL, "JNAM"_rec>;
using MNAM_RACE = Subrecord<raw::MNAM_RACE, "MNAM"_rec>;
using MNAM_SKIL = Subrecord<raw::MNAM_SKIL, "MNAM"_rec>;
using RNAM_NPC_ = Subrecord<raw::RNAM_NPC_, "RNAM"_rec>;
using SNAM_ACTI = Subrecord<raw::SNAM_ACTI, "SNAM"_rec>;
using SNAM_DOOR = Subrecord<raw::SNAM_DOOR, "SNAM"_rec>;
using SNAM_LIGH = Subrecord<raw::SNAM_LIGH, "SNAM"_rec>;
using SNAM_LTEX = Subrecord<raw::SNAM_LTEX, "SNAM"_rec>;
using SNAM_NPC_ = Subrecord<raw::SNAM_NPC_, "SNAM"_rec>;
using SNAM_RACE = Subrecord<raw::SNAM_RACE, "SNAM"_rec>;
using TNAM_DOOR = Subrecord<raw::TNAM_DOOR, "TNAM"_rec>;

DECLARE_SPECIALIZED_SUBRECORD(ACBS);
DECLARE_SPECIALIZED_SUBRECORD(AIDT);
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
