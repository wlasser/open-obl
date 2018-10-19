#ifndef OPENOBLIVION_RECORDS_HPP
#define OPENOBLIVION_RECORDS_HPP

#include "actor_value.hpp"
#include "attribute.hpp"
#include "formid.hpp"
#include "magic_effects.hpp"
#include "record/definition_helpers.hpp"
#include "record/io.hpp"
#include "record/record.hpp"
#include "record/subrecord.hpp"
#include "record/tuplifiable.hpp"
#include "subrecords.hpp"
#include <array>
#include <istream>
#include <ostream>
#include <optional>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

namespace record {

namespace raw {

// This is not a record, but appears multiple times in records with magic
// effect components, e.g ALCH, ENCH, SPEL
struct Effect {
  struct ScriptEffectData {
    // Reverse order compared to Effect
    record::SCIT data{};
    record::FULL name{record::FULL("Script Effect")};
  };
  record::EFID name{};
  record::EFIT data{};
  std::optional<ScriptEffectData> script{};
  uint32_t size() const;
  void read(std::istream &is);
  void write(std::ostream &os) const;
  static bool isNext(std::istream &is);
};

// Full ESM/ESP header
struct TES4 {
  struct Master {
    record::MAST master{};
    record::DATA_TES4 fileSize{};
  };
  record::HEDR header{};
  record::OFST offsets{};
  record::DELE deleted{};
  record::CNAM_TES4 author{};
  record::SNAM description{};
  // Optional TODO: Use std::optional, or leave as an empty vector?
  std::vector<Master> masters{};
};

// Game settings. First character of the editorID determines the type of the
// value; s for string, f for float, and i for int.
struct GMST {
  record::EDID editorID{};
  record::DATA_GMST value{};
};

// Global value. FNAM is essentially meaningless as FLTV is always stored as a
// float bit pattern, even when it is supposed to represent a long, causing loss
// of precision for large values.
struct GLOB {
  record::EDID editorID{};
  record::FNAM_GLOB type{};
  record::FLTV value{};
};

// Player and NPC character class
struct CLAS {
  record::EDID editorID{};
  record::FULL name{};
  // TODO: Use std::optional
  record::DESC description{};
  // TODO: Uses std::optional
  record::ICON iconFilename{};
  record::DATA_CLAS data{};
};

// Faction
// TODO: Look at entire optional behaviour of this record
struct FACT {
  struct Rank {
    record::RNAM index{};
    record::MNAM maleName{};
    record::FNAM_FACT femaleName{};
    record::INAM iconFilename{};
  };
  record::EDID editorID{};
  // TODO: Use std::optional
  record::FULL name{};
  std::vector<record::XNAM> relations{};
  record::DATA_FACT flags{};
  record::CNAM_FACT crimeGoldMultiplier{};
  std::vector<Rank> ranks{};
};

// Hair
// TODO: Look at entire optional structure of this record
struct HAIR {
  record::EDID editorID{};
  record::FULL name{};
  record::MODL modelFilename{};
  record::MODB boundRadius{};
  record::MODT textureHash{};
  record::ICON iconFilename{};
  record::DATA_HAIR flags{};
};

// Eyes
// TODO: Look at entire optional structure of this record
struct EYES {
  record::EDID editorID{};
  record::FULL name{};
  record::ICON iconFilename{};
  record::DATA_EYES flags{};
};

// Character race
struct RACE {
  struct FaceData {
    record::INDX_FACE type{};
    // Instead of simply not including an entry for non-present body parts,
    // such as ears for Argonians, the remaining subrecords are omitted.
    std::optional<record::MODL> modelFilename{};
    std::optional<record::MODB> boundRadius{};
    // Not present for INDX_FACE::eyeLeft and INDX_FACE::eyeRight
    std::optional<record::ICON> textureFilename{};
  };
  struct BodyData {
    record::INDX_BODY type{};
    // Not present for INDX_BODY::tail when the race does not have a tail.
    std::optional<record::ICON> textureFilename{};
  };
  struct TailData {
    record::MODL model{};
    record::MODB boundRadius{};
  };
  record::EDID editorID{};
  std::optional<record::FULL> name{};
  record::DESC description{};
  // FormIDs of greater/lesser powers, racial abilities
  std::vector<record::SPLO> powers{};
  // FormId corresponds to races, not factions
  std::vector<record::XNAM> relations{};
  // Skill modifiers, height, weight, flags
  record::DATA_RACE data{};
  // FormIDs of races that determine the male and female voices.
  // Many races do not have this, including Imperial.
  std::optional<record::VNAM> voices{};
  // Default male and female hair
  std::optional<record::DNAM> defaultHair{};
  // Default hair colour (uint8_t)
  record::CNAM_RACE defaultHairColor{};

  // Facegen main clamp (float)
  std::optional<record::PNAM> facegenMainClamp{};
  // Facegen face clamp (float)
  std::optional<record::UNAM> facegenFaceClamp{};

  record::ATTR baseAttributes{};

  // Face data marker? It's empty.
  record::NAM0 faceMarker{};
  // Face data
  std::vector<FaceData> faceData{};

  // Body data marker
  record::NAM1 bodyMarker{};
  // Male body data
  record::MNAM_RACE maleBodyMarker{};
  std::optional<TailData> maleTailModel{};
  std::vector<BodyData> maleBodyData{};
  // Female body data
  record::FNAM_RACE femaleBodyMarker{};
  std::optional<TailData> femaleTailModel{};
  std::vector<BodyData> femaleBodyData{};

  // Available hair (vector of FormIDs for hair)
  record::HNAM hair{};
  // Available eyes
  record::ENAM eyes{};

  // Facegen data
  record::FGGS fggs{};
  record::FGGA fgga{};
  record::FGTS fgts{};

  // Unused? uin8_t[2]
  record::SNAM_RACE unused{};
};

struct SOUN {
  record::EDID editorID{};
  record::FNAM_SOUN filename{};
  std::variant<record::SNDD, record::SNDX> sound{};
};

struct SKIL {
  record::EDID editorID{};
  record::INDX_SKIL index{};
  record::DESC description{};
  std::optional<record::ICON> iconFilename{};
  record::DATA_SKIL data{};
  record::ANAM_SKIL apprenticeText{};
  record::JNAM_SKIL journeymanText{};
  record::ENAM_SKIL expertText{};
  record::MNAM_SKIL masterText{};
};

struct MGEF {
  // Must be 4 characters TODO: Limit to 4 characters
  record::EDID editorID{};
  record::FULL effectName{};
  record::DESC description{};
  std::optional<record::ICON> iconFilename{};
  std::optional<record::MODL> effectModel{};
  // Always zero
  std::optional<record::MODB> boundRadius{};
  record::DATA_MGEF data{};
  // Editor IDs of magic effects which somehow counter this one, such as Dispel
  // or a Weakness to a Resist. The number of IDs is stored in the data entry.
  record::ESCE counterEffects{};
};

struct LTEX {
  record::EDID editorID{};
  record::ICON textureFilename{};
  std::optional<record::HNAM_LTEX> havokData{};
  std::optional<record::SNAM_LTEX> specularExponent{};
  std::vector<record::GNAM> potentialGrasses{};
};

struct ENCH {
  record::EDID editorID{};
  std::optional<record::FULL> name{};
  record::ENIT_ENCH enchantmentData{};
  std::vector<Effect> effects{};
};

struct SPEL {
  record::EDID editorID{};
  record::FULL name{};
  record::SPIT data{};
  std::vector<Effect> effects{};
};

struct BSGN {
  record::EDID editorID{};
  record::FULL name{};
  record::ICON icon{};
  std::optional<record::DESC> description{};
  std::vector<record::SPLO> spells{};
};

struct DOOR {
  record::EDID editorID{};
  std::optional<record::FULL> name{};
  std::optional<record::MODL> modelFilename{};
  std::optional<record::MODB> boundRadius{};
  std::optional<record::MODT> textureHash{};
  std::optional<record::SCRI> script{};
  std::optional<record::SNAM_DOOR> openSound{};
  std::optional<record::ANAM_DOOR> closeSound{};
  std::optional<record::BNAM_DOOR> loopSound{};
  record::FNAM_DOOR flags{};
  std::vector<record::TNAM_DOOR> randomTeleports{};
};

struct LIGH {
  record::EDID editorID{};
  std::optional<record::MODL> modelFilename{};
  std::optional<record::MODB> boundRadius{};
  std::optional<record::MODT> textureHash{};
  std::optional<record::SCRI> itemScript{};
  std::optional<record::FULL> name{};
  std::optional<record::ICON> icon{};
  record::DATA_LIGH data{};
  std::optional<record::FNAM_LIGH> fadeValue{};
  std::optional<record::SNAM_LIGH> sound{};
};

struct MISC {
  record::EDID editorID{};
  std::optional<record::FULL> name{};
  std::optional<record::MODL> modelFilename{};
  std::optional<record::MODB> boundRadius{};
  std::optional<record::MODT> textureHash{};
  std::optional<record::SCRI> itemScript{};
  std::optional<record::ICON> icon{};
  record::DATA_MISC data{};
};

struct STAT {
  record::EDID editorID{};
  record::MODL modelFilename{};
  record::MODB boundRadius{};
  std::optional<record::MODT> textureHash{};
};

// Potion
struct ALCH {
  // Not present in save games for player-made potions and poisons
  std::optional<record::EDID> editorID{};
  record::FULL itemName{};
  record::MODL modelFilename{};
  std::optional<record::MODB> boundRadius{};
  std::optional<record::MODT> textureHash{};
  std::optional<record::SCRI> itemScript{};
  std::optional<record::ICON> iconFilename{};
  record::DATA_ALCH itemWeight{};
  record::ENIT itemValue{};
  std::vector<Effect> effects{};
};

// The ordering of subrecords is inconsistent. For instance,
// in ICArcaneUniversitySpellmaker XCMT occurs before XOWN,
// in ICTempleDistrictSeridursHouseUpstairs XOWN occurs before XCMT.
// For internal consistency, we destroy the external order and reverse to the
// order below.
struct CELL {
  record::EDID editorID{};
  std::optional<record::FULL> name{};
  record::DATA_CELL data{};
  std::optional<record::XCLL> lighting{};
  std::optional<record::XCMT> music{};
  std::optional<record::XOWN> owner{};
  std::optional<record::XGLB> ownershipGlobal{};
  std::optional<record::XRNK> ownershipRank{};
  std::optional<record::XCCM> climate{};
  std::optional<record::XCLW> waterHeight{};
  std::optional<record::XCWT> water{};
  std::optional<record::XCLR> regions{};
  std::optional<record::XCLC> grid{};
};

// Unsure about the ordering in esm files
struct REFR {
  std::optional<record::EDID> editorID{};
  record::NAME baseID{};
  std::optional<record::DESC> description{};
  std::optional<record::XSCL> scale{};

  std::optional<record::XESP> parent{};
  std::optional<record::XTRG> target{};

  std::optional<record::XPCI> unusedCellID{};
  std::optional<record::FULL> unusedCellName{};

  std::optional<record::XACT> action{};
  std::optional<record::XRGD> ragdollData{};

  // if (marker)
  std::optional<record::XMRK> mapMarker{};
  std::optional<record::FNAM_REFR> mapFlags{};
  std::optional<record::TNAM> markerType{};

  // if (ownable)
  std::optional<record::XOWN> owner{};
  std::optional<record::XGLB> ownershipGlobal{};
  std::optional<record::XRNK> ownershipRank{};

  // if (door)
  std::optional<record::XTEL> teleport{};
  std::optional<record::XRTM> teleportParent{};
  std::optional<record::ONAM> openByDefault{};

  // if (locked)
  std::optional<record::XLOC> lockInfo{};

  // if (tree) ?
  std::optional<record::XSED> speedTree{};
  std::optional<record::XLOD> lod{};

  // if (leveledCreature)
  std::optional<record::XLCM> levelModifier{};

  // if (item)
  std::optional<record::XCNT> count{};

  // if (soulgem)
  std::optional<record::XSOL> soul{};

  // TODO: Finish
  record::DATA_REFR positionRotation{};
};

} // namespace raw

using TES4 = Record<raw::TES4, "TES4"_rec>;
using GMST = Record<raw::GMST, "GMST"_rec>;
using GLOB = Record<raw::GLOB, "GLOB"_rec>;
using CLAS = Record<raw::CLAS, "CLAS"_rec>;
using FACT = Record<raw::FACT, "FACT"_rec>;
using HAIR = Record<raw::HAIR, "HAIR"_rec>;
using EYES = Record<raw::EYES, "EYES"_rec>;
using RACE = Record<raw::RACE, "RACE"_rec>;
using SOUN = Record<raw::SOUN, "SOUN"_rec>;
using SKIL = Record<raw::SKIL, "SKIL"_rec>;
using MGEF = Record<raw::MGEF, "MGEF"_rec>;
using LTEX = Record<raw::LTEX, "LTEX"_rec>;
using ENCH = Record<raw::ENCH, "ENCH"_rec>;
using SPEL = Record<raw::SPEL, "SPEL"_rec>;
using BSGN = Record<raw::BSGN, "BSGN"_rec>;
using DOOR = Record<raw::DOOR, "DOOR"_rec>;
using LIGH = Record<raw::LIGH, "LIGH"_rec>;
using MISC = Record<raw::MISC, "MISC"_rec>;
using STAT = Record<raw::STAT, "STAT"_rec>;
using ALCH = Record<raw::ALCH, "ALCH"_rec>;
using CELL = Record<raw::CELL, "CELL"_rec>;
using REFR = Record<raw::REFR, "REFR"_rec>;

DECLARE_SPECIALIZED_RECORD(TES4);
DECLARE_SPECIALIZED_RECORD(GMST);
DECLARE_SPECIALIZED_RECORD(GLOB);
DECLARE_SPECIALIZED_RECORD(CLAS);
DECLARE_SPECIALIZED_RECORD(FACT);
DECLARE_SPECIALIZED_RECORD(HAIR);
DECLARE_SPECIALIZED_RECORD(EYES);
DECLARE_SPECIALIZED_RECORD(RACE);
DECLARE_SPECIALIZED_RECORD(SOUN);
DECLARE_SPECIALIZED_RECORD(SKIL);
DECLARE_SPECIALIZED_RECORD(MGEF);
DECLARE_SPECIALIZED_RECORD(LTEX);
DECLARE_SPECIALIZED_RECORD(ENCH);
DECLARE_SPECIALIZED_RECORD(SPEL);
DECLARE_SPECIALIZED_RECORD(BSGN);
DECLARE_SPECIALIZED_RECORD(DOOR);
DECLARE_SPECIALIZED_RECORD(LIGH);
DECLARE_SPECIALIZED_RECORD(MISC);
DECLARE_SPECIALIZED_RECORD(STAT);
DECLARE_SPECIALIZED_RECORD(ALCH);
DECLARE_SPECIALIZED_RECORD(CELL);
DECLARE_SPECIALIZED_RECORD(REFR);

} // namespace record

#endif // OPENOBLIVION_RECORDS_HPP
