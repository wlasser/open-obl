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
struct NPC_;
struct ALCH;
struct CELL;

} // namespace raw

template<class T, uint32_t c, bool Compress> class Record;

using TES4 = Record<raw::TES4, "TES4"_rec, false>;
using GMST = Record<raw::GMST, "GMST"_rec, false>;
using GLOB = Record<raw::GLOB, "GLOB"_rec, false>;
using CLAS = Record<raw::CLAS, "CLAS"_rec, false>;
using FACT = Record<raw::FACT, "FACT"_rec, false>;
using HAIR = Record<raw::HAIR, "HAIR"_rec, false>;
using EYES = Record<raw::EYES, "EYES"_rec, false>;
using RACE = Record<raw::RACE, "RACE"_rec, false>;
using SOUN = Record<raw::SOUN, "SOUN"_rec, false>;
using SKIL = Record<raw::SKIL, "SKIL"_rec, false>;
using MGEF = Record<raw::MGEF, "MGEF"_rec, false>;
using LTEX = Record<raw::LTEX, "LTEX"_rec, false>;
using ENCH = Record<raw::ENCH, "ENCH"_rec, false>;
using SPEL = Record<raw::SPEL, "SPEL"_rec, false>;
using BSGN = Record<raw::BSGN, "BSGN"_rec, false>;
using ACTI = Record<raw::ACTI, "ACTI"_rec, false>;
using DOOR = Record<raw::DOOR, "DOOR"_rec, false>;
using LIGH = Record<raw::LIGH, "LIGH"_rec, false>;
using MISC = Record<raw::MISC, "MISC"_rec, false>;
using STAT = Record<raw::STAT, "STAT"_rec, false>;
using NPC_ = Record<raw::NPC_, "NPC_"_rec, true>;
using ALCH = Record<raw::ALCH, "ALCH"_rec, false>;
using CELL = Record<raw::CELL, "CELL"_rec, false>;

} // namespace record

#endif // OPENOBLIVION_RECORDS_FWD_HPP