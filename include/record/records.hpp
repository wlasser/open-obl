#ifndef OPENOBL_RECORDS_HPP
#define OPENOBL_RECORDS_HPP

#include "record/definition_helpers.hpp"
#include "record/record.hpp"
#include "record/reference_records.hpp"
#include "record/subrecords.hpp"
#include <array>
#include <iosfwd>
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
  std::optional<record::OFST> offsets{};
  std::optional<record::DELE> deleted{};
  std::optional<record::CNAM_TES4> author{};
  std::optional<record::SNAM> description{};
  // Optional TODO: Use std::optional, or leave as an empty vector?
  std::vector<Master> masters{};
};

// Game settings. First character of the editorId determines the type of the
// value; s for string, f for float, and i for int.
struct GMST {
  record::EDID editorId{};
  record::DATA_GMST value{};
};

// Global value. FNAM is essentially meaningless as FLTV is always stored as a
// float bit pattern, even when it is supposed to represent a long, causing loss
// of precision for large values.
struct GLOB {
  record::EDID editorId{};
  record::FNAM_GLOB type{};
  record::FLTV value{};
};

// Player and NPC character class
struct CLAS {
  record::EDID editorId{};
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
  record::EDID editorId{};
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
  record::EDID editorId{};
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
  record::EDID editorId{};
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
  record::EDID editorId{};
  std::optional<record::FULL> name{};
  record::DESC description{};
  // FormIds of greater/lesser powers, racial abilities
  std::vector<record::SPLO> powers{};
  // FormId corresponds to races, not factions
  std::vector<record::XNAM> relations{};
  // Skill modifiers, height, weight, flags
  record::DATA_RACE data{};
  // FormIds of races that determine the male and female voices.
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

  // Available hair (vector of FormIds for hair)
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
  record::EDID editorId{};
  record::FNAM_SOUN filename{};
  std::variant<record::SNDD, record::SNDX> sound{};
};

struct SKIL {
  record::EDID editorId{};
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
  record::EDID editorId{};
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
  record::EDID editorId{};
  record::ICON textureFilename{};
  std::optional<record::HNAM_LTEX> havokData{};
  std::optional<record::SNAM_LTEX> specularExponent{};
  std::vector<record::GNAM> potentialGrasses{};
};

struct ENCH {
  std::optional<record::EDID> editorId{};
  std::optional<record::FULL> name{};
  record::ENIT_ENCH enchantmentData{};
  std::vector<Effect> effects{};
};

struct SPEL {
  std::optional<record::EDID> editorId{};
  record::FULL name{};
  record::SPIT data{};
  std::vector<Effect> effects{};
};

struct BSGN {
  record::EDID editorId{};
  record::FULL name{};
  record::ICON icon{};
  std::optional<record::DESC> description{};
  std::vector<record::SPLO> spells{};
};

struct ACTI {
  record::EDID editorId{};
  std::optional<record::FULL> name{};
  std::optional<record::MODL> modelFilename{};
  std::optional<record::MODB> boundRadius{};
  std::optional<record::MODT> textureHash{};
  std::optional<record::SCRI> script{};
  std::optional<record::SNAM_ACTI> sound{};
};

struct CONT {
  record::EDID editorId{};
  std::optional<record::FULL> name{};
  std::optional<record::MODL> modelFilename{};
  std::optional<record::MODB> boundRadius{};
  std::optional<record::MODT> textureHash{};
  std::vector<record::CNTO> items{};
  std::optional<record::DATA_CONT> data{};
  std::optional<record::SNAM_CONT> openSound{};
  std::optional<record::QNAM> closeSound{};
  std::optional<record::SCRI> script{};
};

struct DOOR {
  record::EDID editorId{};
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
  std::optional<record::EDID> editorId{};
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
  std::optional<record::EDID> editorId{};
  std::optional<record::FULL> name{};
  std::optional<record::MODL> modelFilename{};
  std::optional<record::MODB> boundRadius{};
  std::optional<record::MODT> textureHash{};
  std::optional<record::SCRI> itemScript{};
  std::optional<record::ICON> icon{};
  record::DATA_MISC data{};
};

struct STAT {
  record::EDID editorId{};
  record::MODL modelFilename{};
  record::MODB boundRadius{};
  std::optional<record::MODT> textureHash{};
};

struct GRAS {
  record::EDID editorId{};
  record::MODL modelFilename{};
  record::MODB boundRadius{};
  std::optional<record::MODT> textureHash{};
  record::DATA_GRAS data{};
};

struct TREE {
  record::EDID editorId{};
  record::MODL modelFilename{};
  record::MODB boundRadius{};
  std::optional<record::MODT> textureHash{};
  std::optional<record::ICON> leafFilename{};
  std::optional<record::SNAM_TREE> seeds{};
  std::optional<record::CNAM_TREE> data{};
  std::optional<record::BNAM_TREE> billboardDimensions{};
};

struct FLOR {
  record::EDID editorId{};
  std::optional<record::FULL> name{};
  record::MODL modelFilename{};
  record::MODB boundRadius{};
  std::optional<record::MODT> textureHash{};
  std::optional<record::SCRI> script{};
  std::optional<record::PFIG> ingredient{};
  std::optional<record::PFPC> harvestChances{};
};

struct FURN {
  record::EDID editorId{};
  std::optional<record::FULL> name{};
  record::MODL modelFilename{};
  record::MODB boundRadius{};
  std::optional<record::MODT> textureHash{};
  std::optional<record::SCRI> script{};
  record::MNAM_FURN activeMarkers{};
};

struct NPC_ {
  std::optional<record::EDID> editorId{};
  std::optional<record::FULL> name{};
  std::optional<record::MODL> skeletonFilename{};
  std::optional<record::MODB> boundRadius{};
  record::ACBS baseConfig{};
  std::vector<record::SNAM_NPC_> factions{};
  std::optional<record::INAM_NPC_> deathItem{};
  record::RNAM_NPC_ race{};
  std::vector<record::SPLO> spells{};
  std::optional<record::SCRI> script{};
  std::vector<record::CNTO> items{};
  record::AIDT aiData{};
  std::vector<record::PKID> aiPackages{};
  record::CNAM_NPC_ clas{};
  record::DATA_NPC_ stats{};
  std::optional<record::HNAM_NPC_> hair{};
  std::optional<record::LNAM> hairLength{};
  std::optional<record::ENAM_NPC_> eyes{};
  std::optional<record::HCLR> hairColor{};
  std::optional<record::ZNAM> combatStyle{};
  std::optional<record::FGGS> fggs{};
  std::optional<record::FGGA> fgga{};
  std::optional<record::FGTS> fgts{};
  std::optional<record::FNAM_NPC_> fnam{};
};

// Potion
struct ALCH {
  std::optional<record::EDID> editorId{};
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

struct WTHR {
  std::optional<record::EDID> editorId{};
  std::optional<record::CNAM_WTHR> lowerLayerFilename{};
  std::optional<record::DNAM_WTHR> upperLayerFilename{};
  std::optional<record::MODL> precipitationFilename{};
  std::optional<record::MODB> precipitationBoundRadius{};
  std::optional<record::NAM0_WTHR> skyColors{};
  std::optional<record::FNAM_WTHR> fogDistances{};
  std::optional<record::HNAM_WTHR> hdr{};
  std::optional<record::DATA_WTHR> data{};
  std::vector<record::SNAM_WTHR> sounds{};
};

struct CLMT {
  std::optional<record::EDID> editorId{};
  std::optional<record::WLST> weatherList{};
  std::optional<record::FNAM_CLMT> sunFilename{};
  std::optional<record::GNAM_CLMT> sunglareFilename{};
  std::optional<record::MODL> skyFilename{};
  std::optional<record::MODB> boundRadius{};
  std::optional<record::TNAM_CLMT> settings{};
};

// The ordering of subrecords is inconsistent. For instance,
// in ICArcaneUniversitySpellmaker XCMT occurs before XOWN,
// in ICTempleDistrictSeridursHouseUpstairs XOWN occurs before XCMT.
// For internal consistency, we destroy the external order and reverse to the
// order below.
struct CELL {
  // The EDID is optional for exterior cells, where if not present is replaced
  // with 'Wilderness'. To preseve uniqueness of EDIDs, we keep it optional
  // instead of replacing it with the default.
  std::optional<record::EDID> editorId{};
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

struct WRLD {
  record::EDID editorId{};
  std::optional<record::FULL> name{};
  std::optional<record::WNAM> parentWorldspace{};
  std::optional<record::SNAM_WRLD> music{};
  std::optional<record::ICON> mapFilename{};
  std::optional<record::CNAM_WRLD> climate{};
  std::optional<record::NAM2> water{};
  std::optional<record::MNAM_WRLD> mapData{};
  record::DATA_WRLD data{};
  record::NAM0_WRLD bottomLeft{};
  record::NAM9_WRLD topRight{};
};

struct LAND {
  record::DATA_LAND data{};
  std::optional<record::VNML> normals{};
  std::optional<record::VHGT> heights{};
  std::optional<record::VCLR> colors{};
  std::vector<record::BTXT> quadrantTexture{};
  std::vector<std::pair<record::ATXT, record::VTXT>> fineTextures{};
  std::optional<record::VTEX> coarseTextures{};
};

struct WATR {
  record::EDID editorId{};
  std::optional<record::TNAM_WATR> textureFilename{};
  std::optional<record::ANAM_WATR> opacity{};
  std::optional<record::FNAM_WATR> flags{};
  std::optional<record::MNAM_WATR> materialId{};
  std::optional<record::SNAM_WATR> soundId{};
  std::optional<record::DATA_WATR> data{};
  std::optional<record::GNAM_WATR> variants{};
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
using ACTI = Record<raw::ACTI, "ACTI"_rec>;
using CONT = Record<raw::CONT, "CONT"_rec>;
using DOOR = Record<raw::DOOR, "DOOR"_rec>;
using LIGH = Record<raw::LIGH, "LIGH"_rec>;
using MISC = Record<raw::MISC, "MISC"_rec>;
using STAT = Record<raw::STAT, "STAT"_rec>;
using GRAS = Record<raw::GRAS, "GRAS"_rec>;
using TREE = Record<raw::TREE, "TREE"_rec>;
using FLOR = Record<raw::FLOR, "FLOR"_rec>;
using FURN = Record<raw::FURN, "FURN"_rec>;
using NPC_ = Record<raw::NPC_, "NPC_"_rec>;
using ALCH = Record<raw::ALCH, "ALCH"_rec>;
using WTHR = Record<raw::WTHR, "WTHR"_rec>;
using CLMT = Record<raw::CLMT, "CLMT"_rec>;
using CELL = Record<raw::CELL, "CELL"_rec>;
using WRLD = Record<raw::WRLD, "WRLD"_rec>;
using LAND = Record<raw::LAND, "LAND"_rec>;
using WATR = Record<raw::WATR, "WATR"_rec>;

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
DECLARE_SPECIALIZED_RECORD(ACTI);
DECLARE_SPECIALIZED_RECORD(CONT);
DECLARE_SPECIALIZED_RECORD(DOOR);
DECLARE_SPECIALIZED_RECORD(LIGH);
DECLARE_SPECIALIZED_RECORD(MISC);
DECLARE_SPECIALIZED_RECORD(STAT);
DECLARE_SPECIALIZED_RECORD(GRAS);
DECLARE_SPECIALIZED_RECORD(TREE);
DECLARE_SPECIALIZED_RECORD(FLOR);
DECLARE_SPECIALIZED_RECORD(FURN);
DECLARE_SPECIALIZED_RECORD(NPC_);
DECLARE_SPECIALIZED_RECORD(ALCH);
DECLARE_SPECIALIZED_RECORD(WTHR);
DECLARE_SPECIALIZED_RECORD(CLMT);
DECLARE_SPECIALIZED_RECORD(CELL);
DECLARE_SPECIALIZED_RECORD(WRLD);
DECLARE_SPECIALIZED_RECORD(LAND);
DECLARE_SPECIALIZED_RECORD(WATR);

} // namespace Record

#endif // OPENOBL_RECORDS_HPP
