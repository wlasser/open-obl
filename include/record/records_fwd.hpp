#ifndef OPENOBLIVION_RECORDS_FWD_HPP
#define OPENOBLIVION_RECORDS_FWD_HPP

#include <cstdint>
#include "record/rec_of.hpp"

namespace record {

namespace raw {

struct Effect;
struct TES4;
struct GMST;
struct GLOB;
struct CLAS;
struct FACT;
struct HAIR;
struct EYES;
struct RACE;
struct SOUN;
struct SKIL;
struct MGEF;
struct LTEX;
struct ENCH;
struct SPEL;
struct BSGN;
struct ACTI;
struct DOOR;
struct LIGH;
struct MISC;
struct STAT;
struct GRAS;
struct TREE;
struct NPC_;
struct ALCH;
struct WTHR;
struct CLMT;
struct CELL;
struct WRLD;
struct LAND;
struct WATR;

} // namespace raw

template<class T, uint32_t c> class Record;

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
using DOOR = Record<raw::DOOR, "DOOR"_rec>;
using LIGH = Record<raw::LIGH, "LIGH"_rec>;
using MISC = Record<raw::MISC, "MISC"_rec>;
using STAT = Record<raw::STAT, "STAT"_rec>;
using GRAS = Record<raw::GRAS, "GRAS"_rec>;
using TREE = Record<raw::TREE, "TREE"_rec>;
using NPC_ = Record<raw::NPC_, "NPC_"_rec>;
using ALCH = Record<raw::ALCH, "ALCH"_rec>;
using WTHR = Record<raw::WTHR, "WTHR"_rec>;
using CLMT = Record<raw::CLMT, "CLMT"_rec>;
using CELL = Record<raw::CELL, "CELL"_rec>;
using WRLD = Record<raw::WRLD, "WRLD"_rec>;
using LAND = Record<raw::LAND, "LAND"_rec>;
using WATR = Record<raw::WATR, "WATR"_rec>;

} // namespace record

#endif // OPENOBLIVION_RECORDS_FWD_HPP
