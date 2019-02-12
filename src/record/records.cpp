#include "io/io.hpp"
#include "record/io.hpp"
#include "record/records.hpp"
#include <istream>
#include <numeric>
#include <ostream>
#include <set>

namespace record {

using namespace io;

// Effect members
uint32_t raw::Effect::size() const {
  return name.entireSize() + data.entireSize()
      + (script ? (script->data.entireSize() + script->name.entireSize()) : 0u);
}

void raw::Effect::read(std::istream &is) {
  is >> name >> data;
  if (peekRecordType(is) == "SCIT"_rec) {
    ScriptEffectData sed{};
    is >> sed.data >> sed.name;
    script.emplace(std::move(sed));
  }
}

void raw::Effect::write(std::ostream &os) const {
  os << name << data;
  if (script) os << script->data << script->name;
}

bool raw::Effect::isNext(std::istream &is) {
  return peekRecordType(is) == "EFID"_rec;
}

// ALCH specialization
template<> uint32_t ALCH::size() const {
  return SizeOf(editorId)
      + SizeOf(itemName)
      + SizeOf(modelFilename)
      + SizeOf(boundRadius)
      + SizeOf(textureHash)
      + SizeOf(iconFilename)
      + SizeOf(itemScript)
      + SizeOf(itemWeight)
      + SizeOf(itemValue)
      + std::accumulate(effects.begin(), effects.end(), 0u,
                        [](auto a, const auto &b) {
                          return a + b.size();
                        });
}

template<> std::ostream &
raw::write(std::ostream &os, const raw::ALCH &t, std::size_t /*size*/) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.itemName);
  writeRecord(os, t.modelFilename);
  writeRecord(os, t.boundRadius);
  writeRecord(os, t.textureHash);
  writeRecord(os, t.iconFilename);
  writeRecord(os, t.itemScript);
  writeRecord(os, t.itemWeight);
  writeRecord(os, t.itemValue);
  for (const auto &effect : t.effects) effect.write(os);

  return os;
}

template<> std::istream &
raw::read(std::istream &is, raw::ALCH &t, std::size_t /*size*/) {
  readRecord(is, t.editorId);
  readRecord(is, t.itemName);
  readRecord(is, t.modelFilename);
  readRecord(is, t.boundRadius);
  readRecord(is, t.textureHash);
  readRecord(is, t.iconFilename);
  readRecord(is, t.itemScript);
  readRecord(is, t.itemWeight);
  readRecord(is, t.itemValue);
  while (Effect::isNext(is)) t.effects.emplace_back().read(is);

  return is;
}

// TES4 specialization
template<> uint32_t TES4::size() const {
  return SizeOf(header)
      + SizeOf(offsets)
      + SizeOf(deleted)
      + SizeOf(author)
      + SizeOf(description)
      + std::accumulate(masters.begin(), masters.end(), 0u,
                        [](auto a, const auto &b) {
                          return a + SizeOf(b.master) + SizeOf(b.fileSize);
                        });
};

template<> std::ostream &
raw::write(std::ostream &os, const raw::TES4 &t, std::size_t /*size*/) {
  writeRecord(os, t.header);
  writeRecord(os, t.offsets);
  writeRecord(os, t.author);
  writeRecord(os, t.description);
  for (const auto &entry : t.masters) {
    writeRecord(os, entry.master);
    writeRecord(os, entry.fileSize);
  }
  return os;
}

template<> std::istream &
raw::read(std::istream &is, raw::TES4 &t, std::size_t /*size*/) {
  readRecord(is, t.header);
  readRecord(is, t.offsets);
  readRecord(is, t.deleted);
  readRecord(is, t.author);
  readRecord(is, t.description);

  while (peekRecordType(is) == "MAST"_rec) {
    raw::TES4::Master master{};
    is >> master.master;
    readRecord(is, master.fileSize);
    t.masters.push_back(master);
  }
  return is;
}

// GMST specialization
template<> uint32_t GMST::size() const {
  return SizeOf(editorId) + SizeOf(value);
}

template<> std::ostream &
raw::write(std::ostream &os, const raw::GMST &t, std::size_t /*size*/) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.value);
  return os;
}

template<> std::istream &
raw::read(std::istream &is, raw::GMST &t, std::size_t /*size*/) {
  readRecord(is, t.editorId);
  readRecord(is, t.value);
  return is;
}

// GLOB specialization
template<> uint32_t GLOB::size() const {
  return SizeOf(editorId) + SizeOf(type) + SizeOf(value);
}

template<> std::ostream &
raw::write(std::ostream &os, const raw::GLOB &t, std::size_t /*size*/) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.type);
  writeRecord(os, t.value);
  return os;
}

template<> std::istream &
raw::read(std::istream &is, raw::GLOB &t, std::size_t /*size*/) {
  readRecord(is, t.editorId);
  readRecord(is, t.type);
  readRecord(is, t.value);
  return is;
}

// CLAS specialization
template<> uint32_t CLAS::size() const {
  return SizeOf(editorId)
      + SizeOf(name)
      + SizeOf(description)
      + SizeOf(iconFilename)
      + SizeOf(data);
}

template<> std::ostream &
raw::write(std::ostream &os, const raw::CLAS &t, std::size_t /*size*/) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.name);
  writeRecord(os, t.description);
  writeRecord(os, t.iconFilename);
  writeRecord(os, t.data);
  return os;
}

template<> std::istream &
raw::read(std::istream &is, raw::CLAS &t, std::size_t /*size*/) {
  readRecord(is, t.editorId);
  readRecord(is, t.name);
  if (peekRecordType(is) == "DESC"_rec) is >> t.description;
  if (peekRecordType(is) == "ICON"_rec) is >> t.iconFilename;
  readRecord(is, t.data);
  return is;
}

// FACT specialization
template<> uint32_t FACT::size() const {
  return SizeOf(editorId)
      + SizeOf(name)
      + SizeOf(relations)
      + SizeOf(crimeGoldMultiplier)
      + std::accumulate(ranks.begin(), ranks.end(), 0u,
                        [](auto a, const auto &b) {
                          return a
                              + SizeOf(b.index)
                              + SizeOf(b.maleName)
                              + SizeOf(b.femaleName)
                              + SizeOf(b.iconFilename);
                        });
}

template<> std::ostream &
raw::write(std::ostream &os, const raw::FACT &t, std::size_t /*size*/) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.name);
  for (const auto &r : t.relations) writeRecord(os, r);
  writeRecord(os, t.flags);
  writeRecord(os, t.crimeGoldMultiplier);
  for (const auto &r : t.ranks) {
    writeRecord(os, r.index);
    writeRecord(os, r.maleName);
    writeRecord(os, r.femaleName);
    writeRecord(os, r.iconFilename);
  }
  return os;
}

template<> std::istream &
raw::read(std::istream &is, raw::FACT &t, std::size_t /*size*/) {
  readRecord(is, t.editorId);
  if (peekRecordType(is) == "FULL"_rec) is >> t.name;
  while (peekRecordType(is) == "XNAM"_rec) {
    record::XNAM r{};
    is >> r;
    t.relations.push_back(r);
  }
  if (peekRecordType(is) == "DATA"_rec) is >> t.flags;
  if (peekRecordType(is) == "CNAM"_rec) is >> t.crimeGoldMultiplier;
  while (peekRecordType(is) == "RNAM"_rec) {
    raw::FACT::Rank r{};
    is >> r.index;
    if (peekRecordType(is) == "MNAM"_rec) is >> r.maleName;
    if (peekRecordType(is) == "FNAM"_rec) is >> r.femaleName;
    if (peekRecordType(is) == "INAM"_rec) is >> r.iconFilename;
    t.ranks.push_back(r);
  }

  return is;
}

// HAIR specialization
template<> uint32_t HAIR::size() const {
  return SizeOf(editorId)
      + SizeOf(name)
      + SizeOf(modelFilename)
      + SizeOf(boundRadius)
      + SizeOf(textureHash)
      + SizeOf(iconFilename)
      + SizeOf(flags);
}

template<> std::ostream &
raw::write(std::ostream &os, const raw::HAIR &t, std::size_t /*size*/) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.name);
  writeRecord(os, t.modelFilename);
  writeRecord(os, t.boundRadius);
  writeRecord(os, t.textureHash);
  writeRecord(os, t.iconFilename);
  writeRecord(os, t.flags);

  return os;
}

template<> std::istream &
raw::read(std::istream &is, raw::HAIR &t, std::size_t /*size*/) {
  readRecord(is, t.editorId);
  if (peekRecordType(is) == "FULL"_rec) is >> t.name;
  if (peekRecordType(is) == "MODL"_rec) is >> t.modelFilename;
  if (peekRecordType(is) == "MODB"_rec) is >> t.boundRadius;
  if (peekRecordType(is) == "MODT"_rec) is >> t.textureHash;
  if (peekRecordType(is) == "ICON"_rec) is >> t.iconFilename;
  if (peekRecordType(is) == "DATA"_rec) is >> t.flags;

  return is;
}

// EYES specialization
template<> uint32_t EYES::size() const {
  return SizeOf(editorId) + SizeOf(name) + SizeOf(iconFilename) + SizeOf(flags);
}

template<> std::ostream &
raw::write(std::ostream &os, const raw::EYES &t, std::size_t /*size*/) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.name);
  writeRecord(os, t.iconFilename);
  writeRecord(os, t.flags);

  return os;
}

template<> std::istream &
raw::read(std::istream &is, raw::EYES &t, std::size_t /*size*/) {
  readRecord(is, t.editorId);
  if (peekRecordType(is) == "FULL"_rec) is >> t.name;
  if (peekRecordType(is) == "ICON"_rec) is >> t.iconFilename;
  if (peekRecordType(is) == "DATA"_rec) is >> t.flags;

  return is;
}

// RACE specialization
template<> uint32_t RACE::size() const {
  return SizeOf(editorId)
      + SizeOf(name)
      + SizeOf(description)
      + SizeOf(powers)
      + SizeOf(relations)
      + SizeOf(data)
      + SizeOf(voices)
      + SizeOf(defaultHair)
      + SizeOf(defaultHairColor)
      + SizeOf(facegenMainClamp)
      + SizeOf(facegenFaceClamp)
      + SizeOf(baseAttributes)
      + SizeOf(faceMarker)
      + std::accumulate(faceData.begin(), faceData.end(), 0u,
                        [](auto a, const auto &b) {
                          return a + SizeOf(b.type)
                              + SizeOf(b.modelFilename)
                              + SizeOf(b.boundRadius)
                              + SizeOf(b.textureFilename);
                        })
      + SizeOf(bodyMarker)
      + SizeOf(maleBodyMarker)
      + [&t = maleTailModel]() {
        return t ? (SizeOf(t->model) + SizeOf(t->boundRadius)) : 0u;
      }()
      + std::accumulate(maleBodyData.begin(), maleBodyData.end(), 0u,
                        [](auto a, const auto &b) {
                          return a + SizeOf(b.type) + SizeOf(b.textureFilename);
                        })
      + SizeOf(femaleBodyMarker)
      + [&t = femaleTailModel]() {
        return t ? (SizeOf(t->model) + SizeOf(t->boundRadius)) : 0u;
      }()
      + std::accumulate(femaleBodyData.begin(), femaleBodyData.end(), 0u,
                        [](auto a, const auto &b) {
                          return a + SizeOf(b.type) + SizeOf(b.textureFilename);
                        })
      + SizeOf(hair)
      + SizeOf(eyes)
      + SizeOf(fggs)
      + SizeOf(fgga)
      + SizeOf(fgts)
      + SizeOf(unused);
}

template<> std::ostream &
raw::write(std::ostream &os, const raw::RACE &t, std::size_t /*size*/) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.name);
  writeRecord(os, t.description);
  for (const auto &power : t.powers) writeRecord(os, power);
  for (const auto &relation : t.relations) writeRecord(os, relation);
  writeRecord(os, t.data);
  writeRecord(os, t.voices);
  writeRecord(os, t.defaultHair);
  writeRecord(os, t.defaultHairColor);
  writeRecord(os, t.facegenMainClamp);
  writeRecord(os, t.facegenFaceClamp);
  writeRecord(os, t.baseAttributes);

  writeRecord(os, t.faceMarker);
  for (const auto &faceData : t.faceData) {
    writeRecord(os, faceData.type);
    writeRecord(os, faceData.modelFilename);
    writeRecord(os, faceData.boundRadius);
    writeRecord(os, faceData.textureFilename);
  }

  writeRecord(os, t.bodyMarker);
  writeRecord(os, t.maleBodyMarker);
  if (t.maleTailModel) {
    writeRecord(os, t.maleTailModel->model);
    writeRecord(os, t.maleTailModel->boundRadius);
  }
  for (const auto &bodyData : t.maleBodyData) {
    writeRecord(os, bodyData.type);
    writeRecord(os, bodyData.textureFilename);
  }

  writeRecord(os, t.femaleBodyMarker);
  if (t.femaleTailModel) {
    writeRecord(os, t.femaleTailModel->model);
    writeRecord(os, t.femaleTailModel->boundRadius);
  }
  for (const auto &bodyData : t.femaleBodyData) {
    writeRecord(os, bodyData.type);
    writeRecord(os, bodyData.textureFilename);
  }

  writeRecord(os, t.hair);
  writeRecord(os, t.eyes);
  writeRecord(os, t.fggs);
  writeRecord(os, t.fgga);
  writeRecord(os, t.fgts);
  writeRecord(os, t.unused);

  return os;
}

template<> std::istream &
raw::read(std::istream &is, raw::RACE &t, std::size_t /*size*/) {
  readRecord(is, t.editorId);
  readRecord(is, t.name);
  readRecord(is, t.description);

  while (peekRecordType(is) == "SPLO"_rec) {
    record::SPLO r{};
    is >> r;
    t.powers.push_back(r);
  }

  while (peekRecordType(is) == "XNAM"_rec) {
    record::XNAM r{};
    is >> r;
    t.relations.push_back(r);
  }

  readRecord(is, t.data);
  readRecord(is, t.voices);
  readRecord(is, t.defaultHair);
  readRecord(is, t.defaultHairColor);
  readRecord(is, t.facegenMainClamp);
  readRecord(is, t.facegenFaceClamp);
  readRecord(is, t.baseAttributes);
  readRecord(is, t.faceMarker);

  while (peekRecordType(is) == "INDX"_rec) {
    raw::RACE::FaceData f{};
    is >> f.type;
    readRecord(is, f.modelFilename);
    readRecord(is, f.boundRadius);
    readRecord(is, f.textureFilename);
    t.faceData.push_back(f);
  }

  readRecord(is, t.bodyMarker);
  readRecord(is, t.maleBodyMarker);

  if (peekRecordType(is) == "MODL"_rec) {
    raw::RACE::TailData tail{};
    is >> tail.model;
    readRecord(is, tail.boundRadius);
    t.maleTailModel.emplace(tail);
  } else {
    t.maleTailModel = std::nullopt;
  }

  while (peekRecordType(is) == "INDX"_rec) {
    raw::RACE::BodyData b{};
    is >> b.type;
    readRecord(is, b.textureFilename);
    t.maleBodyData.push_back(b);
  }

  readRecord(is, t.femaleBodyMarker);

  if (peekRecordType(is) == "MODL"_rec) {
    raw::RACE::TailData tail{};
    is >> tail.model;
    readRecord(is, tail.boundRadius);
    t.femaleTailModel.emplace(tail);
  } else {
    t.femaleTailModel = std::nullopt;
  }

  while (peekRecordType(is) == "INDX"_rec) {
    raw::RACE::BodyData b{};
    is >> b.type;
    readRecord(is, b.textureFilename);
    t.femaleBodyData.push_back(b);
  }

  readRecord(is, t.hair);
  readRecord(is, t.eyes);
  readRecord(is, t.fggs);
  readRecord(is, t.fgga);
  readRecord(is, t.fgts);
  readRecord(is, t.unused);

  return is;
}

// SOUN specialization
template<> uint32_t SOUN::size() const {
  return SizeOf(editorId)
      + SizeOf(filename)
      + std::visit([](const auto &r) { return SizeOf(r); }, sound);
}

template<> std::ostream &
raw::write(std::ostream &os, const raw::SOUN &t, std::size_t /*size*/) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.filename);
  std::visit([&os](const auto &r) { writeRecord(os, r); }, t.sound);

  return os;
}

template<> std::istream &
raw::read(std::istream &is, raw::SOUN &t, std::size_t /*size*/) {
  readRecord(is, t.editorId);
  readRecord(is, t.filename);

  if (peekRecordType(is) == "SNDD"_rec) {
    record::SNDD r{};
    is >> r;
    t.sound.emplace<0>(r);
  } else if (peekRecordType(is) == "SNDX"_rec) {
    record::SNDX r{};
    is >> r;
    t.sound.emplace<1>(r);
  }
  return is;
}

// SKIL specialization
template<> uint32_t SKIL::size() const {
  return SizeOf(editorId)
      + SizeOf(index)
      + SizeOf(iconFilename)
      + SizeOf(data)
      + SizeOf(apprenticeText)
      + SizeOf(journeymanText)
      + SizeOf(expertText)
      + SizeOf(masterText);
}

template<> std::ostream &
raw::write(std::ostream &os, const raw::SKIL &t, std::size_t /*size*/) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.index);
  writeRecord(os, t.description);
  writeRecord(os, t.iconFilename);
  writeRecord(os, t.data);
  writeRecord(os, t.apprenticeText);
  writeRecord(os, t.journeymanText);
  writeRecord(os, t.expertText);
  writeRecord(os, t.masterText);

  return os;
}

template<> std::istream &
raw::read(std::istream &is, raw::SKIL &t, std::size_t /*size*/) {
  readRecord(is, t.editorId);
  readRecord(is, t.index);
  readRecord(is, t.description);
  readRecord(is, t.iconFilename);
  readRecord(is, t.data);
  readRecord(is, t.apprenticeText);
  readRecord(is, t.journeymanText);
  readRecord(is, t.expertText);
  readRecord(is, t.masterText);

  return is;
}

// MGEF specialization
template<> uint32_t MGEF::size() const {
  return SizeOf(editorId)
      + SizeOf(effectName)
      + SizeOf(description)
      + SizeOf(iconFilename)
      + SizeOf(effectModel)
      + SizeOf(boundRadius)
      + SizeOf(data)
      + SizeOf(counterEffects);
}

template<> std::ostream &
raw::write(std::ostream &os, const raw::MGEF &t, std::size_t /*size*/) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.effectName);
  writeRecord(os, t.description);
  writeRecord(os, t.iconFilename);
  writeRecord(os, t.effectModel);
  writeRecord(os, t.boundRadius);
  writeRecord(os, t.data);
  writeRecord(os, t.counterEffects);

  return os;
}

template<> std::istream &
raw::read(std::istream &is, raw::MGEF &t, std::size_t /*size*/) {
  readRecord(is, t.editorId);
  readRecord(is, t.effectName);
  readRecord(is, t.description);
  readRecord(is, t.iconFilename);
  readRecord(is, t.effectModel);
  readRecord(is, t.boundRadius);
  readRecord(is, t.data);
  if (peekRecordType(is) == "ESCE"_rec) is >> t.counterEffects;

  return is;
}

// LTEX specialization
template<> uint32_t LTEX::size() const {
  return SizeOf(editorId)
      + SizeOf(textureFilename)
      + SizeOf(havokData)
      + SizeOf(specularExponent)
      + SizeOf(potentialGrasses);
}

template<> std::ostream &
raw::write(std::ostream &os, const raw::LTEX &t, std::size_t /*size*/) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.textureFilename);
  writeRecord(os, t.havokData);
  writeRecord(os, t.specularExponent);
  for (const auto &grass : t.potentialGrasses) writeRecord(os, grass);

  return os;
}

template<> std::istream &
raw::read(std::istream &is, raw::LTEX &t, std::size_t size) {
  readRecord(is, t.editorId);
  readRecord(is, t.textureFilename);
  readRecord(is, t.havokData);
  readRecord(is, t.specularExponent);
  while (peekRecordType(is) == "GNAM"_rec) {
    record::GNAM r{};
    is >> r;
    t.potentialGrasses.push_back(r);
  }
  return is;
}

// STAT specialization
template<> uint32_t STAT::size() const {
  return SizeOf(editorId)
      + SizeOf(modelFilename)
      + SizeOf(boundRadius)
      + SizeOf(textureHash);
}

template<> std::ostream &
raw::write(std::ostream &os, const raw::STAT &t, std::size_t /*size*/) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.modelFilename);
  writeRecord(os, t.boundRadius);
  writeRecord(os, t.textureHash);

  return os;
}

template<> std::istream &
raw::read(std::istream &is, raw::STAT &t, std::size_t size) {
  // There are a few corrupted records making this harder than it needs to be:
  // ARVineRising02 has no MODL, MODB, or MODT
  // Empty STAT (size 0) after PalaceDRug01
  if (size == 0) return is;
  readRecord(is, t.editorId);
  if (peekRecordType(is) != "MODL"_rec) return is;
  readRecord(is, t.modelFilename);
  readRecord(is, t.boundRadius);
  readRecord(is, t.textureHash);

  return is;
}

// ENCH specialization
template<> uint32_t ENCH::size() const {
  return SizeOf(editorId)
      + SizeOf(name)
      + SizeOf(enchantmentData)
      + std::accumulate(effects.begin(), effects.end(), 0,
                        [](auto a, const auto &b) { return a + b.size(); });
}

template<> std::ostream &
raw::write(std::ostream &os, const raw::ENCH &t, std::size_t /*size*/) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.name);
  writeRecord(os, t.enchantmentData);
  for (const auto &effect : t.effects) effect.write(os);

  return os;
}

template<> std::istream &
raw::read(std::istream &is, raw::ENCH &t, std::size_t /*size*/) {
  readRecord(is, t.editorId);
  readRecord(is, t.name);
  readRecord(is, t.enchantmentData);
  while (Effect::isNext(is)) t.effects.emplace_back().read(is);

  return is;
}

// SPEL specialization
template<> uint32_t SPEL::size() const {
  return SizeOf(editorId)
      + SizeOf(name)
      + SizeOf(data)
      + std::accumulate(effects.begin(), effects.end(), 0u,
                        [](auto a, const auto &b) { return a + b.size(); });
}

template<> std::ostream &
raw::write(std::ostream &os, const raw::SPEL &t, std::size_t /*size*/) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.name);
  writeRecord(os, t.data);
  for (const auto &effect : t.effects) effect.write(os);

  return os;
}

template<> std::istream &
raw::read(std::istream &is, raw::SPEL &t, std::size_t /*size*/) {
  readRecord(is, t.editorId);
  readRecord(is, t.name);
  readRecord(is, t.data);
  while (Effect::isNext(is)) t.effects.emplace_back().read(is);

  return is;
}

// CELL specialization
template<> uint32_t CELL::size() const {
  return SizeOf(editorId)
      + SizeOf(name)
      + SizeOf(data)
      + SizeOf(lighting)
      + SizeOf(music)
      + SizeOf(owner)
      + SizeOf(ownershipGlobal)
      + SizeOf(ownershipRank)
      + SizeOf(climate)
      + SizeOf(water)
      + SizeOf(waterHeight)
      + SizeOf(regions)
      + SizeOf(grid);
}

template<> std::ostream &
raw::write(std::ostream &os, const raw::CELL &t, std::size_t size) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.name);
  writeRecord(os, t.data);
  writeRecord(os, t.lighting);
  writeRecord(os, t.music);
  writeRecord(os, t.owner);
  writeRecord(os, t.ownershipGlobal);
  writeRecord(os, t.ownershipRank);
  writeRecord(os, t.climate);
  writeRecord(os, t.waterHeight);
  writeRecord(os, t.water);
  writeRecord(os, t.regions);
  writeRecord(os, t.grid);

  return os;
}

template<> std::istream &
raw::read(std::istream &is, raw::CELL &t, std::size_t size) {
  readRecord(is, t.editorId);
  readRecord(is, t.name);
  readRecord(is, t.data);
  std::set<uint32_t> possibleSubrecords = {
      "XCLL"_rec, "XOWN"_rec, "XGLB"_rec, "XRNK"_rec, "XCMT"_rec,
      "XCCM"_rec, "XCLW"_rec, "XCWT"_rec, "XCLR"_rec, "XCLC"_rec
  };
  uint32_t rec{};
  while (possibleSubrecords.count(rec = peekRecordType(is)) == 1) {
    switch (rec) {
      case "XCLL"_rec:readRecord(is, t.lighting);
        break;
      case "XOWN"_rec:readRecord(is, t.owner);
        break;
      case "XGLB"_rec:readRecord(is, t.ownershipGlobal);
        break;
      case "XRNK"_rec:readRecord(is, t.ownershipRank);
        break;
      case "XCMT"_rec:readRecord(is, t.music);
        break;
      case "XCCM"_rec:readRecord(is, t.climate);
        break;
      case "XCLW"_rec:readRecord(is, t.waterHeight);
        break;
      case "XCWT"_rec:readRecord(is, t.water);
        break;
      case "XCLR"_rec:readRecord(is, t.regions);
        break;
      case "XCLC"_rec:readRecord(is, t.grid);
        break;
      default: return is;
    }
  }
  return is;
}

// WRLD specialization
template<> uint32_t WRLD::size() const {
  return SizeOf(editorId)
      + SizeOf(name)
      + SizeOf(parentWorldspace)
      + SizeOf(music)
      + SizeOf(mapFilename)
      + SizeOf(climate)
      + SizeOf(water)
      + SizeOf(mapData)
      + SizeOf(data)
      + SizeOf(bottomLeft)
      + SizeOf(topRight);
}

template<> std::ostream &
raw::write(std::ostream &os, const raw::WRLD &t, std::size_t size) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.name);
  writeRecord(os, t.parentWorldspace);
  writeRecord(os, t.music);
  writeRecord(os, t.mapFilename);
  writeRecord(os, t.climate);
  writeRecord(os, t.water);
  writeRecord(os, t.mapData);
  writeRecord(os, t.data);
  writeRecord(os, t.bottomLeft);
  writeRecord(os, t.topRight);

  return os;
}

template<> std::istream &
raw::read(std::istream &is, raw::WRLD &t, std::size_t size) {
  readRecord(is, t.editorId);
  std::set<uint32_t> possibleSubrecords = {
      "FULL"_rec, "WNAM"_rec, "SNAM"_rec, "ICON"_rec,
      "CNAM"_rec, "NAM2"_rec, "MNAM"_rec,
  };
  uint32_t rec{};
  while (possibleSubrecords.count(rec = peekRecordType(is)) == 1) {
    switch (rec) {
      case "FULL"_rec:readRecord(is, t.name);
        break;
      case "WNAM"_rec:readRecord(is, t.parentWorldspace);
        break;
      case "SNAM"_rec:readRecord(is, t.music);
        break;
      case "ICON"_rec:readRecord(is, t.mapFilename);
        break;
      case "CNAM"_rec:readRecord(is, t.climate);
        break;
      case "NAM2"_rec:readRecord(is, t.water);
        break;
      case "MNAM"_rec:readRecord(is, t.mapData);
        break;
      default: break;
    }
  }

  readRecord(is, t.data);
  readRecord(is, t.bottomLeft);
  readRecord(is, t.topRight);

  // We don't care about the annoying OFST record, so skip over it.
  if (peekRecordType(is) == "XXXX"_rec) {
    const uint32_t ofstSize{readRecord<record::XXXX>(is).data};
    is.seekg(ofstSize + 6u, std::ios_base::cur);
  } else if (peekRecordType(is) == "OFST"_rec) {
    skipRecord(is);
  }

  return is;
}

// LIGH specialization
template<> uint32_t LIGH::size() const {
  return SizeOf(editorId)
      + SizeOf(modelFilename)
      + SizeOf(boundRadius)
      + SizeOf(textureHash)
      + SizeOf(itemScript)
      + SizeOf(name)
      + SizeOf(icon)
      + SizeOf(data)
      + SizeOf(fadeValue)
      + SizeOf(sound);
}

template<> std::ostream &
raw::write(std::ostream &os, const raw::LIGH &t, std::size_t/*size*/) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.modelFilename);
  writeRecord(os, t.boundRadius);
  writeRecord(os, t.textureHash);
  writeRecord(os, t.itemScript);
  writeRecord(os, t.name);
  writeRecord(os, t.icon);
  writeRecord(os, t.data);
  writeRecord(os, t.fadeValue);
  writeRecord(os, t.sound);

  return os;
}

template<> std::istream &
raw::read(std::istream &is, raw::LIGH &t, std::size_t /*size*/) {
  readRecord(is, t.editorId);
  std::set<uint32_t> possibleSubrecords = {
      "MODL"_rec, "MODB"_rec, "MODT"_rec, "SCRI"_rec, "FULL"_rec, "ICON"_rec
  };
  uint32_t rec;
  while (possibleSubrecords.count(rec = peekRecordType(is)) == 1) {
    switch (rec) {
      case "MODL"_rec:readRecord(is, t.modelFilename);
        break;
      case "MODB"_rec:readRecord(is, t.boundRadius);
        break;
      case "MODT"_rec:readRecord(is, t.textureHash);
        break;
      case "SCRI"_rec:readRecord(is, t.itemScript);
        break;
      case "FULL"_rec:readRecord(is, t.name);
        break;
      case "ICON"_rec:readRecord(is, t.icon);
        break;
      default:break;
    }
  }

  readRecord(is, t.data);
  readRecord(is, t.fadeValue);
  readRecord(is, t.sound);

  return is;
}

// BSGN specialization
template<> uint32_t BSGN::size() const {
  return SizeOf(editorId)
      + SizeOf(name)
      + SizeOf(icon)
      + SizeOf(description)
      + SizeOf(spells);
}

template<> std::ostream &
raw::write(std::ostream &os, const raw::BSGN &t, std::size_t /*size*/) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.name);
  writeRecord(os, t.icon);
  writeRecord(os, t.description);
  for (const auto &spell : t.spells) writeRecord(os, spell);

  return os;
}

template<> std::istream &
raw::read(std::istream &is, raw::BSGN &t, std::size_t /*size*/) {
  readRecord(is, t.editorId);
  readRecord(is, t.name);
  readRecord(is, t.icon);
  readRecord(is, t.description);
  while (peekRecordType(is) == "SPLO"_rec) {
    record::SPLO r{};
    is >> r;
    t.spells.push_back(r);
  }
  return is;
}

// MISC specialization
template<> uint32_t MISC::size() const {
  return SizeOf(editorId)
      + SizeOf(name)
      + SizeOf(modelFilename)
      + SizeOf(boundRadius)
      + SizeOf(textureHash)
      + SizeOf(itemScript)
      + SizeOf(icon)
      + SizeOf(data);
}

template<> std::ostream &
raw::write(std::ostream &os, const raw::MISC &t, std::size_t /*size*/) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.name);
  writeRecord(os, t.modelFilename);
  writeRecord(os, t.boundRadius);
  writeRecord(os, t.textureHash);
  writeRecord(os, t.icon);
  writeRecord(os, t.itemScript);
  writeRecord(os, t.data);

  return os;
}

template<> std::istream &
raw::read(std::istream &is, raw::MISC &t, std::size_t /*size*/) {
  readRecord(is, t.editorId);
  readRecord(is, t.name);
  readRecord(is, t.modelFilename);
  readRecord(is, t.boundRadius);
  readRecord(is, t.textureHash);
  readRecord(is, t.icon);
  readRecord(is, t.itemScript);
  readRecord(is, t.data);

  return is;
}

// DOOR specialization
template<> uint32_t DOOR::size() const {
  return SizeOf(editorId)
      + SizeOf(name)
      + SizeOf(modelFilename)
      + SizeOf(boundRadius)
      + SizeOf(textureHash)
      + SizeOf(script)
      + SizeOf(openSound)
      + SizeOf(closeSound)
      + SizeOf(loopSound)
      + SizeOf(flags)
      + SizeOf(randomTeleports);
}

template<> std::ostream &
raw::write(std::ostream &os, const raw::DOOR &t, std::size_t /*size*/) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.name);
  writeRecord(os, t.modelFilename);
  writeRecord(os, t.boundRadius);
  writeRecord(os, t.textureHash);
  writeRecord(os, t.script);
  writeRecord(os, t.openSound);
  writeRecord(os, t.closeSound);
  writeRecord(os, t.loopSound);
  writeRecord(os, t.flags);
  for (const auto &rec : t.randomTeleports) writeRecord(os, rec);

  return os;
}

template<> std::istream &
raw::read(std::istream &is, raw::DOOR &t, std::size_t /*size*/) {
  readRecord(is, t.editorId);
  readRecord(is, t.name);
  readRecord(is, t.modelFilename);
  readRecord(is, t.boundRadius);
  readRecord(is, t.textureHash);
  readRecord(is, t.script);
  readRecord(is, t.openSound);
  readRecord(is, t.closeSound);
  readRecord(is, t.loopSound);
  readRecord(is, t.flags);
  while (peekRecordType(is) == "TNAM"_rec) {
    record::TNAM_DOOR r{};
    is >> r;
    t.randomTeleports.push_back(r);
  }

  return is;
}

// ACTI specialization
template<> uint32_t ACTI::size() const {
  return SizeOf(editorId)
      + SizeOf(name)
      + SizeOf(modelFilename)
      + SizeOf(boundRadius)
      + SizeOf(textureHash)
      + SizeOf(script)
      + SizeOf(sound);
}

template<> std::ostream &
raw::write(std::ostream &os, const raw::ACTI &t, std::size_t /*size*/) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.name);
  writeRecord(os, t.modelFilename);
  writeRecord(os, t.boundRadius);
  writeRecord(os, t.textureHash);
  writeRecord(os, t.script);
  writeRecord(os, t.sound);

  return os;
}

template<> std::istream &
raw::read(std::istream &is, raw::ACTI &t, std::size_t /*size*/) {
  readRecord(is, t.editorId);
  readRecord(is, t.name);
  readRecord(is, t.modelFilename);
  readRecord(is, t.boundRadius);
  readRecord(is, t.textureHash);
  readRecord(is, t.script);
  readRecord(is, t.sound);

  return is;
}

// NPC_ specialization
template<> uint32_t NPC_::size() const {
  return SizeOf(editorId)
      + SizeOf(name)
      + SizeOf(skeletonFilename)
      + SizeOf(boundRadius)
      + SizeOf(baseConfig)
      + SizeOf(factions)
      + SizeOf(deathItem)
      + SizeOf(race)
      + SizeOf(spells)
      + SizeOf(script)
      + SizeOf(items)
      + SizeOf(aiData)
      + SizeOf(aiPackages)
      + SizeOf(clas)
      + SizeOf(stats)
      + SizeOf(hair)
      + SizeOf(hairLength)
      + SizeOf(eyes)
      + SizeOf(hairColor)
      + SizeOf(combatStyle)
      + SizeOf(fggs)
      + SizeOf(fgga)
      + SizeOf(fgga)
      + SizeOf(fgts)
      + SizeOf(fnam);
}

template<> std::ostream &
raw::write(std::ostream &os, const raw::NPC_ &t, std::size_t size) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.name);
  writeRecord(os, t.skeletonFilename);
  writeRecord(os, t.boundRadius);
  writeRecord(os, t.baseConfig);
  for (const auto &faction : t.factions) writeRecord(os, faction);
  writeRecord(os, t.deathItem);
  writeRecord(os, t.race);
  for (const auto &spell : t.spells) writeRecord(os, spell);
  writeRecord(os, t.script);
  for (const auto &item : t.items) writeRecord(os, item);
  writeRecord(os, t.aiData);
  for (const auto &package : t.aiPackages) writeRecord(os, package);
  writeRecord(os, t.clas);
  writeRecord(os, t.stats);
  writeRecord(os, t.hair);
  writeRecord(os, t.hairLength);
  writeRecord(os, t.eyes);
  writeRecord(os, t.hairColor);
  writeRecord(os, t.combatStyle);
  writeRecord(os, t.fggs);
  writeRecord(os, t.fgga);
  writeRecord(os, t.fgts);
  writeRecord(os, t.fnam);

  return os;
}

template<> std::istream &
raw::read(std::istream &is, raw::NPC_ &t, std::size_t size) {
  readRecord(is, t.editorId);
  std::set<uint32_t> possibleSubrecords = {
      "FULL"_rec, "MODL"_rec, "MODB"_rec, "ACBS"_rec, "SNAM"_rec, "INAM"_rec,
      "RNAM"_rec, "SPLO"_rec, "SCRI"_rec, "CNTO"_rec, "AIDT"_rec, "PKID"_rec,
      "CNAM"_rec, "DATA"_rec, "HNAM"_rec, "LNAM"_rec, "ENAM"_rec, "HCLR"_rec,
      "ZNAM"_rec, "FGGS"_rec, "FGGA"_rec, "FGTS"_rec, "FNAM"_rec,
  };

  uint32_t rec{};
  while (possibleSubrecords.count(rec = peekRecordType(is)) == 1) {
    switch (rec) {
      case "FULL"_rec: readRecord(is, t.name);
        break;
      case "MODL"_rec: readRecord(is, t.skeletonFilename);
        break;
      case "MODB"_rec: readRecord(is, t.boundRadius);
        break;
      case "ACBS"_rec: readRecord(is, t.baseConfig);
        break;
      case "SNAM"_rec: readRecord(is, t.factions.emplace_back());
        break;
      case "INAM"_rec: readRecord(is, t.deathItem);
        break;
      case "RNAM"_rec: readRecord(is, t.race);
        break;
      case "SPLO"_rec: readRecord(is, t.spells.emplace_back());
        break;
      case "SCRI"_rec: readRecord(is, t.script);
        break;
      case "CNTO"_rec: readRecord(is, t.items.emplace_back());
        break;
      case "AIDT"_rec: readRecord(is, t.aiData);
        break;
      case "PKID"_rec: readRecord(is, t.aiPackages.emplace_back());
        break;
      case "CNAM"_rec: readRecord(is, t.clas);
        break;
      case "DATA"_rec: readRecord(is, t.stats);
        break;
      case "HNAM"_rec: readRecord(is, t.hair);
        break;
      case "LNAM"_rec: readRecord(is, t.hairLength);
        break;
      case "ENAM"_rec: readRecord(is, t.eyes);
        break;
      case "HCLR"_rec: readRecord(is, t.hairColor);
        break;
      case "ZNAM"_rec: readRecord(is, t.combatStyle);
        break;
      case "FGGS"_rec: readRecord(is, t.fggs);
        break;
      case "FGGA"_rec: readRecord(is, t.fgga);
        break;
      case "FGTS"_rec: readRecord(is, t.fgts);
        break;
      case "FNAM"_rec: readRecord(is, t.fnam);
        break;
      default: break;
    }
  }

  (void) 0;
  return is;
}

template<> std::ostream &
raw::write(std::ostream &os, const raw::REFR_ACTI &t, std::size_t) {
  return t.write(os);
}

template<> std::istream &
raw::read(std::istream &is, raw::REFR_ACTI &t, std::size_t) {
  return t.read(is);
}

template<> std::ostream &
raw::write(std::ostream &os, const raw::REFR_DOOR &t, std::size_t) {
  return t.write(os);
}

template<> std::istream &
raw::read(std::istream &is, raw::REFR_DOOR &t, std::size_t) {
  return t.read(is);
}

template<> std::ostream &
raw::write(std::ostream &os, const raw::REFR_LIGH &t, std::size_t) {
  return t.write(os);
}

template<> std::istream &
raw::read(std::istream &is, raw::REFR_LIGH &t, std::size_t) {
  return t.read(is);
}

template<> std::ostream &
raw::write(std::ostream &os, const raw::REFR_MISC &t, std::size_t) {
  return t.write(os);
}

template<> std::istream &
raw::read(std::istream &is, raw::REFR_MISC &t, std::size_t) {
  return t.read(is);
}

template<> std::ostream &
raw::write(std::ostream &os, const raw::REFR_STAT &t, std::size_t) {
  return t.write(os);
}

template<> std::istream &
raw::read(std::istream &is, raw::REFR_STAT &t, std::size_t) {
  return t.read(is);
}

template<> std::ostream &
raw::write(std::ostream &os, const raw::REFR_NPC_ &t, std::size_t) {
  return t.write(os);
}

template<> std::istream &
raw::read(std::istream &is, raw::REFR_NPC_ &t, std::size_t) {
  return t.read(is);
}

oo::BaseId peekBaseOfReference(std::istream &is) {
  const auto start{is.tellg()};
  raw::REFRBase refr{};
  readRecordHeader(is);
  refr.read(is);
  is.seekg(start, std::ios::beg);
  return refr.baseId.data;
}

} // namespace record
