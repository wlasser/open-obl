#include "record/record.hpp"
#include "records.hpp"
#include "subrecords.hpp"
#include "io/write_bytes.hpp"
#include "io/read_bytes.hpp"
#include <iostream>
#include <memory>
#include <numeric>
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
  if (peekRecordType(is) == "SCIT") {
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
  return peekRecordType(is) == "EFID";
}

// ALCH specialization
template<>
uint32_t ALCH::size() const {
  return (data.editorID ? data.editorID->entireSize() : 0u)
      + data.itemName.entireSize()
      + data.modelFilename.entireSize()
      + (data.boundRadius ? data.boundRadius->entireSize() : 0u)
      + (data.textureHash ? data.textureHash->entireSize() : 0u)
      + (data.iconFilename ? data.iconFilename->entireSize() : 0u)
      + (data.itemScript ? data.itemScript->entireSize() : 0u)
      + data.itemWeight.entireSize()
      + data.itemValue.entireSize()
      + std::accumulate(data.effects.begin(), data.effects.end(), 0u,
                        [](auto a, const auto &b) {
                          return a + b.size();
                        });
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::ALCH &t, std::size_t /*size*/) {
  if (t.editorID) os << *t.editorID;
  os << t.itemName << t.modelFilename;
  if (t.boundRadius) os << *t.boundRadius;
  if (t.textureHash) os << *t.textureHash;
  if (t.iconFilename) os << *t.iconFilename;
  if (t.itemScript) os << *t.itemScript;
  os << t.itemWeight << t.itemValue;
  for (const auto &effect : t.effects) effect.write(os);

  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::ALCH &t, std::size_t /*size*/) {
  readRecord(is, t.editorID, "EDID");
  readRecord(is, t.itemName, "FULL");
  readRecord(is, t.modelFilename, "MODL");
  readRecord(is, t.boundRadius, "MODB");
  readRecord(is, t.textureHash, "MODT");
  readRecord(is, t.iconFilename, "ICON");
  readRecord(is, t.itemScript, "SCRI");
  readRecord(is, t.itemWeight, "DATA");
  readRecord(is, t.itemValue, "ENIT");
  while (Effect::isNext(is)) t.effects.emplace_back().read(is);

  return is;
}

// TES4 specialization
template<>
uint32_t TES4::size() const {
  uint32_t size = data.header.entireSize() + data.offsets.entireSize()
      + data.deleted.entireSize() + data.author.entireSize()
      + data.description.entireSize();
  for (const auto &master : data.masters) {
    size += master.master.entireSize() + master.fileSize.entireSize();
  }
  return size;
};

template<>
std::ostream &
raw::write(std::ostream &os, const raw::TES4 &t, std::size_t /*size*/) {
  os << t.header << t.offsets << t.deleted << t.author << t.description;
  for (const auto &master : t.masters) {
    os << master.master << master.fileSize;
  }
  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::TES4 &t, std::size_t /*size*/) {
  readRecord(is, t.header, "HEDR");
  readRecord(is, t.offsets, "OFST");
  readRecord(is, t.deleted, "DELE");
  readRecord(is, t.author, "CNAM");
  readRecord(is, t.description, "SNAM");

  while (peekRecordType(is) == "MAST") {
    raw::TES4::Master master{};
    is >> master.master;
    readRecord(is, master.fileSize, "DATA");
    t.masters.push_back(master);
  }
  return is;
}

// GMST specialization
template<>
uint32_t GMST::size() const {
  return data.editorID.entireSize() + data.value.entireSize();
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::GMST &t, std::size_t /*size*/) {
  os << t.editorID << t.value;
  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::GMST &t, std::size_t /*size*/) {
  readRecord(is, t.editorID, "EDID");
  readRecord(is, t.value, "DATA");
  return is;
}

// GLOB specialization
template<>
uint32_t GLOB::size() const {
  return data.editorID.entireSize() + data.type.entireSize()
      + data.value.entireSize();
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::GLOB &t, std::size_t /*size*/) {
  os << t.editorID << t.type << t.value;
  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::GLOB &t, std::size_t /*size*/) {
  readRecord(is, t.editorID, "EDID");
  readRecord(is, t.type, "FNAM");
  readRecord(is, t.value, "FLTV");
  return is;
}

// CLAS specialization
template<>
uint32_t CLAS::size() const {
  return data.editorID.entireSize() + data.name.entireSize()
      + data.description.entireSize() + data.iconFilename.entireSize()
      + data.data.entireSize();
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::CLAS &t, std::size_t /*size*/) {
  os << t.editorID << t.name << t.description << t.iconFilename << t.data;
  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::CLAS &t, std::size_t /*size*/) {
  readRecord(is, t.editorID, "EDID");
  readRecord(is, t.name, "FULL");
  if (peekRecordType(is) == "DESC") is >> t.description;
  if (peekRecordType(is) == "ICON") is >> t.iconFilename;
  readRecord(is, t.data, "DATA");
  return is;
}

// FACT specialization
template<>
uint32_t FACT::size() const {
  uint32_t size = data.editorID.entireSize() + data.name.entireSize()
      + data.flags.entireSize() + data.crimeGoldMultiplier.entireSize();
  for (const auto &r : data.relations) {
    size += r.entireSize();
  }
  for (const auto &r : data.ranks) {
    size += r.index.entireSize() + r.maleName.entireSize()
        + r.femaleName.entireSize() + r.iconFilename.entireSize();
  }
  return size;
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::FACT &t, std::size_t /*size*/) {
  os << t.editorID << t.name;
  for (const auto &r : t.relations) {
    os << r;
  }
  os << t.flags << t.crimeGoldMultiplier;
  for (const auto &r : t.ranks) {
    os << r.index << r.maleName << r.femaleName << r.iconFilename;
  }
  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::FACT &t, std::size_t /*size*/) {
  readRecord(is, t.editorID, "EDID");
  if (peekRecordType(is) == "FULL") is >> t.name;
  while (peekRecordType(is) == "XNAM") {
    record::XNAM r{};
    is >> r;
    t.relations.push_back(r);
  }
  if (peekRecordType(is) == "DATA") is >> t.flags;
  if (peekRecordType(is) == "CNAM") is >> t.crimeGoldMultiplier;
  while (peekRecordType(is) == "RNAM") {
    raw::FACT::Rank r{};
    is >> r.index;
    if (peekRecordType(is) == "MNAM") is >> r.maleName;
    if (peekRecordType(is) == "FNAM") is >> r.femaleName;
    if (peekRecordType(is) == "INAM") is >> r.iconFilename;
    t.ranks.push_back(r);
  }

  return is;
}

// HAIR specialization
template<>
uint32_t HAIR::size() const {
  return data.editorID.entireSize() + data.name.entireSize()
      + data.modelFilename.entireSize() + data.boundRadius.entireSize()
      + data.boundRadius.entireSize() + data.textureHash.entireSize()
      + data.iconFilename.entireSize() + data.flags.entireSize();
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::HAIR &t, std::size_t /*size*/) {
  os << t.editorID << t.name << t.modelFilename << t.boundRadius
     << t.textureHash << t.iconFilename << t.flags;
  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::HAIR &t, std::size_t /*size*/) {
  readRecord(is, t.editorID, "EDID");
  if (peekRecordType(is) == "FULL") is >> t.name;
  if (peekRecordType(is) == "MODL") is >> t.modelFilename;
  if (peekRecordType(is) == "MODB") is >> t.boundRadius;
  if (peekRecordType(is) == "MODT") is >> t.textureHash;
  if (peekRecordType(is) == "ICON") is >> t.iconFilename;
  if (peekRecordType(is) == "DATA") is >> t.flags;

  return is;
}

// EYES specialization
template<>
uint32_t EYES::size() const {
  return data.editorID.entireSize() + data.name.entireSize()
      + data.iconFilename.entireSize() + data.flags.entireSize();
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::EYES &t, std::size_t /*size*/) {
  os << t.editorID << t.name << t.iconFilename << t.flags;
  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::EYES &t, std::size_t /*size*/) {
  readRecord(is, t.editorID, "EDID");
  if (peekRecordType(is) == "FULL") is >> t.name;
  if (peekRecordType(is) == "ICON") is >> t.iconFilename;
  if (peekRecordType(is) == "DATA") is >> t.flags;

  return is;
}

// RACE specialization
template<>
uint32_t RACE::size() const {
  uint32_t size = data.editorID.entireSize()
      + (data.name ? data.name->entireSize() : 0u)
      + data.description.entireSize();
  for (const auto &power : data.powers) size += power.entireSize();
  for (const auto &relation : data.relations) size += relation.entireSize();
  size += data.data.entireSize();
  if (data.voices) size += data.voices.value().entireSize();
  if (data.defaultHair) size += data.defaultHair.value().entireSize();
  size += data.defaultHairColor.entireSize()
      + (data.facegenMainClamp ? data.facegenMainClamp->entireSize() : 0u)
      + (data.facegenFaceClamp ? data.facegenFaceClamp->entireSize() : 0u)
      + data.baseAttributes.entireSize() + data.faceMarker.entireSize();
  for (const auto &faceData : data.faceData) {
    size += faceData.type.entireSize()
        + (faceData.modelFilename ? faceData.modelFilename->entireSize() : 0u)
        + (faceData.boundRadius ? faceData.boundRadius->entireSize() : 0u)
        + (faceData.textureFilename ? faceData.textureFilename->entireSize()
                                    : 0u);
  }
  size += data.bodyMarker.entireSize() + data.maleBodyMarker.entireSize();
  if (data.maleTailModel) {
    size += data.maleTailModel->model.entireSize()
        + data.maleTailModel->boundRadius.entireSize();
  }
  for (const auto &bodyData : data.maleBodyData) {
    size += bodyData.type.entireSize();
    if (bodyData.textureFilename) {
      size += bodyData.textureFilename.value().entireSize();
    }
  }
  size += data.femaleBodyMarker.entireSize();
  if (data.femaleTailModel) {
    size += data.femaleTailModel->model.entireSize()
        + data.femaleTailModel->boundRadius.entireSize();
  }
  for (const auto &bodyData : data.femaleBodyData) {
    size += bodyData.type.entireSize();
    if (bodyData.textureFilename) {
      size += bodyData.textureFilename.value().entireSize();
    }
  }
  size += data.hair.entireSize() + data.eyes.entireSize()
      + data.fggs.entireSize() + data.fgga.entireSize() + data.fgts.entireSize()
      + data.unused.entireSize();
  return size;
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::RACE &t, std::size_t /*size*/) {
  os << t.editorID;
  if (t.name) os << t.name.value();
  os << t.description;
  for (const auto &power : t.powers) os << power;
  for (const auto &relation : t.relations) os << relation;
  os << t.data;
  if (t.voices) os << t.voices.value();
  if (t.defaultHair) os << t.defaultHair.value();
  os << t.defaultHairColor;
  if (t.facegenMainClamp) os << t.facegenMainClamp.value();
  if (t.facegenFaceClamp) os << t.facegenFaceClamp.value();
  os << t.baseAttributes << t.faceMarker;
  for (const auto &faceData : t.faceData) {
    os << faceData.type;
    if (faceData.modelFilename) os << faceData.modelFilename.value();
    if (faceData.boundRadius) os << faceData.boundRadius.value();
    if (faceData.textureFilename) os << faceData.textureFilename.value();
  }
  os << t.bodyMarker << t.maleBodyMarker;
  if (t.maleTailModel) {
    os << t.maleTailModel->model << t.maleTailModel->boundRadius;
  }
  for (const auto &bodyData : t.maleBodyData) {
    os << bodyData.type;
    if (bodyData.textureFilename) os << bodyData.textureFilename.value();
  }
  os << t.femaleBodyMarker;
  if (t.femaleTailModel) {
    os << t.femaleTailModel->model << t.femaleTailModel->boundRadius;
  }
  for (const auto &bodyData : t.femaleBodyData) {
    os << bodyData.type;
    if (bodyData.textureFilename) os << bodyData.textureFilename.value();
  }
  os << t.hair << t.eyes << t.fggs << t.fgga << t.fgts << t.unused;

  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::RACE &t, std::size_t /*size*/) {
  readRecord(is, t.editorID, "EDID");
  readRecord(is, t.name, "FULL");
  readRecord(is, t.description, "DESC");

  while (peekRecordType(is) == "SPLO") {
    record::SPLO r{};
    is >> r;
    t.powers.push_back(r);
  }

  while (peekRecordType(is) == "XNAM") {
    record::XNAM r{};
    is >> r;
    t.relations.push_back(r);
  }

  readRecord(is, t.data, "DATA");
  readRecord(is, t.voices, "VNAM");
  readRecord(is, t.defaultHair, "DNAM");
  readRecord(is, t.defaultHairColor, "CNAM");
  readRecord(is, t.facegenMainClamp, "PNAM");
  readRecord(is, t.facegenFaceClamp, "UNAM");
  readRecord(is, t.baseAttributes, "ATTR");
  readRecord(is, t.faceMarker, "NAM0");

  while (peekRecordType(is) == "INDX") {
    raw::RACE::FaceData f{};
    is >> f.type;
    readRecord(is, f.modelFilename, "MODL");
    readRecord(is, f.boundRadius, "MODB");
    readRecord(is, f.textureFilename, "ICON");
    t.faceData.push_back(f);
  }

  readRecord(is, t.bodyMarker, "NAM1");
  readRecord(is, t.maleBodyMarker, "MNAM");

  if (peekRecordType(is) == "MODL") {
    raw::RACE::TailData tail{};
    is >> tail.model;
    readRecord(is, tail.boundRadius, "MODB");
    t.maleTailModel.emplace(tail);
  } else {
    t.maleTailModel = std::nullopt;
  }

  while (peekRecordType(is) == "INDX") {
    raw::RACE::BodyData b{};
    is >> b.type;
    readRecord(is, b.textureFilename, "ICON");
    t.maleBodyData.push_back(b);
  }

  readRecord(is, t.femaleBodyMarker, "FNAM");

  if (peekRecordType(is) == "MODL") {
    raw::RACE::TailData tail{};
    is >> tail.model;
    readRecord(is, tail.boundRadius, "MODB");
    t.femaleTailModel.emplace(tail);
  } else {
    t.femaleTailModel = std::nullopt;
  }

  while (peekRecordType(is) == "INDX") {
    raw::RACE::BodyData b{};
    is >> b.type;
    readRecord(is, b.textureFilename, "ICON");
    t.femaleBodyData.push_back(b);
  }

  readRecord(is, t.hair, "HNAM");
  readRecord(is, t.eyes, "ENAM");
  readRecord(is, t.fggs, "FGGS");
  readRecord(is, t.fgga, "FGGA");
  readRecord(is, t.fgts, "FGTS");
  readRecord(is, t.unused, "SNAM");

  return is;
}

// SOUN specialization
template<>
uint32_t SOUN::size() const {
  return data.editorID.entireSize() + data.filename.entireSize()
      + std::visit([](auto &&r) {
        using T = std::decay_t<decltype(r)>;
        if constexpr (std::is_same_v<T, record::SNDD>) {
          return r.entireSize();
        } else if constexpr (std::is_same_v<T, record::SNDX>) {
          return r.entireSize();
        } else {
          return 0;
        }
      }, data.sound);
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::SOUN &t, std::size_t /*size*/) {
  os << t.editorID << t.filename;
  std::visit([&os](auto &&r) {
    using T = std::decay_t<decltype(r)>;
    if constexpr (std::is_same_v<T, record::SNDD>) {
      os << r;
    } else if constexpr(std::is_same_v<T, record::SNDX>) {
      os << r;
    }
  }, t.sound);
  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::SOUN &t, std::size_t /*size*/) {
  readRecord(is, t.editorID, "EDID");
  readRecord(is, t.filename, "FNAM");

  if (peekRecordType(is) == "SNDD") {
    record::SNDD r{};
    is >> r;
    t.sound.emplace<0>(r);
  } else if (peekRecordType(is) == "SNDX") {
    record::SNDX r{};
    is >> r;
    t.sound.emplace<1>(r);
  }
  return is;
}

// SKIL specialization
template<>
uint32_t SKIL::size() const {
  return data.editorID.entireSize() + data.index.entireSize()
      + data.description.entireSize()
      + (data.iconFilename ? data.iconFilename->entireSize() : 0u)
      + data.data.entireSize() + data.apprenticeText.entireSize()
      + data.journeymanText.entireSize() + data.expertText.entireSize()
      + data.expertText.entireSize();
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::SKIL &t, std::size_t /*size*/) {
  os << t.editorID << t.index << t.description;
  if (t.iconFilename) os << t.iconFilename.value();
  os << t.data << t.apprenticeText << t.journeymanText << t.expertText
     << t.masterText;
  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::SKIL &t, std::size_t /*size*/) {
  readRecord(is, t.editorID, "EDID");
  readRecord(is, t.index, "INDX");
  readRecord(is, t.description, "DESC");
  readRecord(is, t.iconFilename, "ICON");
  readRecord(is, t.data, "DATA");
  readRecord(is, t.apprenticeText, "ANAM");
  readRecord(is, t.journeymanText, "JNAM");
  readRecord(is, t.expertText, "ENAM");
  readRecord(is, t.masterText, "MNAM");

  return is;
}

// MGEF specialization
template<>
uint32_t MGEF::size() const {
  return data.editorID.entireSize() + data.effectName.entireSize()
      + data.description.entireSize()
      + (data.iconFilename ? data.iconFilename->entireSize() : 0u)
      + (data.effectModel ? data.effectModel->entireSize() : 0u)
      + (data.boundRadius ? data.boundRadius->entireSize() : 0u)
      + data.data.entireSize() + data.counterEffects.entireSize();
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::MGEF &t, std::size_t /*size*/) {
  os << t.editorID << t.effectName << t.description;
  if (t.iconFilename) os << t.iconFilename.value();
  if (t.effectModel) os << t.effectModel.value();
  if (t.boundRadius) os << t.boundRadius.value();
  os << t.data << t.counterEffects;

  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::MGEF &t, std::size_t /*size*/) {
  readRecord(is, t.editorID, "EDID");
  readRecord(is, t.effectName, "FULL");
  readRecord(is, t.description, "DESC");
  readRecord(is, t.iconFilename, "ICON");
  readRecord(is, t.effectModel, "MODL");
  readRecord(is, t.boundRadius, "MODB");
  readRecord(is, t.data, "DATA");
  if (peekRecordType(is) == "ESCE") is >> t.counterEffects;

  return is;
}

// LTEX specialization
template<>
uint32_t LTEX::size() const {
  uint32_t size = data.editorID.entireSize() + data.textureFilename.entireSize()
      + (data.havokData ? data.havokData->entireSize() : 0u)
      + (data.specularExponent ? data.specularExponent->entireSize() : 0u);
  for (const auto &grass : data.potentialGrasses) {
    size += grass.entireSize();
  }
  return size;
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::LTEX &t, std::size_t /*size*/) {
  os << t.editorID << t.textureFilename;
  if (t.havokData) os << t.havokData.value();
  if (t.specularExponent) os << t.specularExponent.value();
  for (const auto &grass : t.potentialGrasses) {
    os << grass;
  }

  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::LTEX &t, std::size_t size) {
  readRecord(is, t.editorID, "EDID");
  readRecord(is, t.textureFilename, "ICON");
  readRecord(is, t.havokData, "HNAM");
  readRecord(is, t.specularExponent, "SNAM");
  while (peekRecordType(is) == "GNAM") {
    record::GNAM r{};
    is >> r;
    t.potentialGrasses.push_back(r);
  }
  return is;
}

// STAT specialization
template<>
uint32_t STAT::size() const {
  return data.editorID.entireSize() + data.modelFilename.entireSize()
      + data.boundRadius.entireSize()
      + (data.textureHash ? data.textureHash->entireSize() : 0u);
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::STAT &t, std::size_t /*size*/) {
  os << t.editorID << t.modelFilename << t.boundRadius;
  if (t.textureHash) os << t.textureHash.value();
  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::STAT &t, std::size_t size) {
  // There are a few corrupted records making this harder than it needs to be:
  // ARVineRising02 has no MODL, MODB, or MODT
  // Empty STAT (size 0) after PalaceDRug01
  if (size == 0) return is;
  readRecord(is, t.editorID, "EDID");
  if (peekRecordType(is) != "MODL") return is;
  readRecord(is, t.modelFilename, "MODL");
  readRecord(is, t.boundRadius, "MODB");
  readRecord(is, t.textureHash, "MODT");

  return is;
}

// ENCH specialization
template<>
uint32_t ENCH::size() const {
  return data.editorID.entireSize()
      + (data.name ? data.name->entireSize() : 0u)
      + data.enchantmentData.entireSize()
      + std::accumulate(data.effects.begin(), data.effects.end(), 0,
                        [](auto a, const auto &b) {
                          return a + b.size();
                        });
}

template<>
std::istream &raw::read(std::istream &is, raw::ENCH &t, std::size_t /*size*/) {
  readRecord(is, t.editorID, "EDID");
  readRecord(is, t.name, "FULL");
  readRecord(is, t.enchantmentData, "DATA");
  while (Effect::isNext(is)) t.effects.emplace_back().read(is);

  return is;
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::ENCH &t, std::size_t /*size*/) {
  os << t.editorID;
  if (t.name) os << *t.name;
  os << t.enchantmentData;
  for (const auto &effect : t.effects) effect.write(os);

  return os;
}

// SPEL specialization
template<>
uint32_t SPEL::size() const {
  return data.editorID.entireSize()
      + data.name.entireSize()
      + data.data.entireSize()
      + std::accumulate(data.effects.begin(), data.effects.end(), 0u,
                        [](auto a, const auto &b) {
                          return a + b.size();
                        });
}

template<>
std::istream &raw::read(std::istream &is, raw::SPEL &t, std::size_t /*size*/) {
  readRecord(is, t.editorID, "EDID");
  readRecord(is, t.name, "FULL");
  readRecord(is, t.data, "SPIT");
  while (Effect::isNext(is)) t.effects.emplace_back().read(is);

  return is;
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::SPEL &t, std::size_t /*size*/) {
  os << t.editorID << t.name << t.data;
  for (const auto &effect : t.effects) effect.write(os);

  return os;
}

// CELL specialization
template<>
uint32_t CELL::size() const {
  return data.editorID.entireSize()
      + (data.name ? data.name->entireSize() : 0u)
      + data.data.entireSize()
      + (data.lighting ? data.lighting->entireSize() : 0u)
      + (data.music ? data.music->entireSize() : 0u)
      + (data.owner ? data.owner->entireSize() : 0u)
      + (data.ownershipGlobal ? data.ownershipGlobal->entireSize() : 0u)
      + (data.ownershipRank ? data.ownershipRank->entireSize() : 0u)
      + (data.climate ? data.climate->entireSize() : 0u)
      + (data.water ? data.water->entireSize() : 0u)
      + (data.waterHeight ? data.waterHeight->entireSize() : 0u)
      + (data.regions ? data.regions->entireSize() : 0u)
      + (data.grid ? data.grid->entireSize() : 0u);
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::CELL &t, std::size_t size) {
  os << t.editorID;
  if (t.name) os << t.name.value();
  os << t.data;
  if (t.lighting) os << t.lighting.value();
  if (t.music) os << t.music.value();
  if (t.owner) os << t.owner.value();
  if (t.ownershipGlobal) os << t.ownershipGlobal.value();
  if (t.ownershipRank) os << t.ownershipRank.value();
  if (t.climate) os << t.climate.value();
  if (t.waterHeight) os << t.waterHeight.value();
  if (t.water) os << t.water.value();
  if (t.regions) os << t.regions.value();
  if (t.grid) os << t.grid.value();
  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::CELL &t, std::size_t size) {
  readRecord(is, t.editorID, "EDID");
  readRecord(is, t.name, "FULL");
  readRecord(is, t.data, "DATA");
  std::set<std::string> possibleSubrecords = {
      "XCLL", "XOWN", "XGLB", "XRNK", "XCMT",
      "XCCM", "XCLW", "XCWT", "XCLR", "XCLC"
  };
  std::string rec;
  while (possibleSubrecords.count(rec = peekRecordType(is)) == 1) {
    // Convert to an integer to switch over
    if (rec.length() != 4) {
      throw std::runtime_error(
          std::string("Expected a subrecord type, found ").append(rec));
    }
    std::array<char, 4> recordArray = {rec[0], rec[1], rec[2], rec[3]};
    switch (recOf(recordArray)) {
      case "XCLL"_rec:readRecord(is, t.lighting, "XCLL");
        break;
      case "XOWN"_rec:readRecord(is, t.owner, "XOWN");
        break;
      case "XGLB"_rec:readRecord(is, t.ownershipGlobal, "XGLB");
        break;
      case "XRNK"_rec:readRecord(is, t.ownershipRank, "XRNK");
        break;
      case "XCMT"_rec:readRecord(is, t.music, "XCMT");
        break;
      case "XCCM"_rec:readRecord(is, t.climate, "XCCM");
        break;
      case "XCLW"_rec:readRecord(is, t.waterHeight, "XCLW");
        break;
      case "XCWT"_rec:readRecord(is, t.water, "XCWT");
        break;
      case "XCLR"_rec:readRecord(is, t.regions, "XCLR");
        break;
      case "XCLC"_rec:readRecord(is, t.grid, "XCLC");
        break;
      default: return is;
    }
  }
  return is;
}

// REFR specialization
template<>
uint32_t REFR::size() const {
  return data.baseID.entireSize()
      + (data.editorID ? data.editorID->entireSize() : 0u)
      + (data.description ? data.description->entireSize() : 0u)
      + (data.scale ? data.scale->entireSize() : 0u)
      + (data.parent ? data.parent->entireSize() : 0u)
      + (data.target ? data.target->entireSize() : 0u)
      + (data.unusedCellID ? data.unusedCellID->entireSize() : 0u)
      + (data.unusedCellName ? data.unusedCellName->entireSize() : 0u)
      + (data.action ? data.action->entireSize() : 0u)
      + (data.ragdollData ? data.ragdollData->entireSize() : 0u)
      + (data.mapMarker ? data.mapFlags->entireSize() : 0u)
      + (data.mapFlags ? data.mapFlags->entireSize() : 0u)
      + (data.markerType ? data.markerType->entireSize() : 0u)
      + (data.owner ? data.owner->entireSize() : 0u)
      + (data.ownershipGlobal ? data.ownershipGlobal->entireSize() : 0u)
      + (data.ownershipRank ? data.ownershipRank->entireSize() : 0u)
      + (data.teleport ? data.teleport->entireSize() : 0u)
      + (data.teleportParent ? data.teleportParent->entireSize() : 0u)
      + (data.openByDefault ? data.openByDefault->entireSize() : 0u)
      + (data.lockInfo ? data.lockInfo->entireSize() : 0u)
      + (data.speedTree ? data.speedTree->entireSize() : 0u)
      + (data.lod ? data.lod->entireSize() : 0u)
      + (data.levelModifier ? data.levelModifier->entireSize() : 0u)
      + (data.count ? data.count->entireSize() : 0u)
      + (data.soul ? data.soul->entireSize() : 0u)
      + data.positionRotation.entireSize();
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::REFR &t, std::size_t size) {
  os << t.baseID;
  if (t.editorID) os << *t.editorID;
  if (t.description) os << *t.description;
  if (t.scale) os << *t.scale;
  if (t.parent) os << *t.parent;
  if (t.target) os << *t.target;
  if (t.unusedCellID) os << *t.unusedCellID;
  if (t.unusedCellName) os << *t.unusedCellName;
  if (t.action) os << *t.action;
  if (t.ragdollData) os << *t.ragdollData;
  if (t.mapMarker) os << *t.mapMarker;
  if (t.mapFlags) os << *t.mapFlags;
  if (t.markerType) os << *t.markerType;
  if (t.owner) os << *t.owner;
  if (t.ownershipGlobal) os << *t.ownershipGlobal;
  if (t.ownershipRank) os << *t.ownershipRank;
  if (t.teleport) os << *t.teleport;
  if (t.teleportParent) os << *t.teleportParent;
  if (t.openByDefault) os << *t.openByDefault;
  if (t.lockInfo) os << *t.lockInfo;
  if (t.speedTree) os << *t.speedTree;
  if (t.lod) os << *t.lod;
  if (t.levelModifier) os << *t.levelModifier;
  if (t.count) os << *t.count;
  if (t.soul) os << *t.soul;
  os << t.positionRotation;

  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::REFR &t, std::size_t size) {
  readRecord(is, t.editorID, "EDID");
  readRecord(is, t.baseID, "NAME");
  std::set<std::string> possibleSubrecords = {
      "DESC", "XSCL", "XESP", "XTRG",
      "XPCI", "FULL", "XACT", "XRGD", "XMRK",
      "FNAM", "TNAM", "XOWN", "XGLB", "XRNK",
      "XTEL", "XRTM", "ONAM", "XLOC", "XSED",
      "XLOD", "XLCM", "XCNT", "XSOL"
  };
  std::string rec;
  while (possibleSubrecords.count(rec = peekRecordType(is)) == 1) {
    if (rec.length() != 4) {
      throw std::runtime_error(
          std::string("Expected a subrecord type, found ").append(rec));
    }
    std::array<char, 4> recordArray = {rec[0], rec[1], rec[2], rec[3]};
    switch (recOf(recordArray)) {
      case "DESC"_rec:readRecord(is, t.description, "DESC");
        break;
      case "XSCL"_rec:readRecord(is, t.scale, "XSCL");
        break;
      case "XESP"_rec:readRecord(is, t.parent, "XESP");
        break;
      case "XTRG"_rec:readRecord(is, t.target, "XTRG");
        break;
      case "XPCI"_rec:readRecord(is, t.unusedCellID, "XPCI");
        break;
      case "FULL"_rec:readRecord(is, t.unusedCellName, "FULL");
        break;
      case "XACT"_rec:readRecord(is, t.action, "XACT");
        break;
      case "XRGD"_rec:readRecord(is, t.ragdollData, "XRGD");
        break;
      case "XMRK"_rec:readRecord(is, t.mapMarker, "XMRK");
        break;
      case "FNAM"_rec:readRecord(is, t.mapFlags, "FNAM");
        break;
      case "TNAM"_rec:readRecord(is, t.markerType, "TNAM");
        break;
      case "XOWN"_rec:readRecord(is, t.owner, "XOWN");
        break;
      case "XGLB"_rec:readRecord(is, t.ownershipGlobal, "XGLB");
        break;
      case "XRNK"_rec:readRecord(is, t.ownershipRank, "XRNK");
        break;
      case "XTEL"_rec:readRecord(is, t.teleport, "XTEL");
        break;
      case "XRTM"_rec:readRecord(is, t.teleportParent, "XRTM");
        break;
      case "ONAM"_rec:readRecord(is, t.openByDefault, "ONAM");
        break;
      case "XLOC"_rec:readRecord(is, t.lockInfo, "XLOC");
        break;
      case "XSED"_rec:readRecord(is, t.speedTree, "XSED");
        break;
      case "XLOD"_rec:readRecord(is, t.lod, "XLOD");
        break;
      case "XLCM"_rec:readRecord(is, t.levelModifier, "XLCM");
        break;
      case "XCNT"_rec:readRecord(is, t.count, "XCNT");
        break;
      case "XSOL"_rec:readRecord(is, t.soul, "XSOL");
        break;
      default:break;
    }
  }

  readRecord(is, t.positionRotation, "DATA");
  return is;
}

// LIGH specialization
template<>
uint32_t LIGH::size() const {
  return data.editorID.entireSize()
      + (data.modelFilename ? data.modelFilename->entireSize() : 0u)
      + (data.boundRadius ? data.boundRadius->entireSize() : 0u)
      + (data.textureHash ? data.textureHash->entireSize() : 0u)
      + (data.itemScript ? data.itemScript->entireSize() : 0u)
      + (data.name ? data.name->entireSize() : 0u)
      + (data.icon ? data.icon->entireSize() : 0u)
      + data.data.entireSize()
      + (data.fadeValue ? data.fadeValue->entireSize() : 0u)
      + (data.sound ? data.sound->entireSize() : 0u);
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::LIGH &t, std::size_t/*size*/) {
  os << t.editorID;
  if (t.modelFilename) os << *t.modelFilename;
  if (t.boundRadius) os << *t.boundRadius;
  if (t.textureHash) os << *t.textureHash;
  if (t.itemScript) os << *t.itemScript;
  if (t.name) os << *t.name;
  if (t.icon) os << *t.icon;
  os << t.data;
  if (t.fadeValue) os << *t.fadeValue;
  if (t.sound) os << *t.sound;

  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::LIGH &t, std::size_t /*size*/) {
  readRecord(is, t.editorID, "EDID");
  std::set<std::string> possibleSubrecords = {
      "MODL", "MODB", "MODT", "SCRI", "FULL", "ICON"
  };
  std::string rec;
  while (possibleSubrecords.count(rec = peekRecordType(is)) == 1) {
    if (rec.length() != 4) {
      throw std::runtime_error(
          std::string("Expected a subrecord type, found ").append(rec));
    }
    std::array<char, 4> recordArray = {rec[0], rec[1], rec[2], rec[3]};
    switch (recOf(recordArray)) {
      case "MODL"_rec:readRecord(is, t.modelFilename, "MODL");
        break;
      case "MODB"_rec:readRecord(is, t.boundRadius, "MODB");
        break;
      case "MODT"_rec:readRecord(is, t.textureHash, "MODT");
        break;
      case "SCRI"_rec:readRecord(is, t.itemScript, "SCRI");
        break;
      case "FULL"_rec:readRecord(is, t.name, "FULL");
        break;
      case "ICON"_rec:readRecord(is, t.icon, "ICON");
        break;
      default:break;
    }
  }

  readRecord(is, t.data, "DATA");
  readRecord(is, t.fadeValue, "FNAM");
  readRecord(is, t.sound, "SNAM");

  return is;
}

// BSGN specialization
template<>
uint32_t BSGN::size() const {
  return data.editorID.entireSize()
      + data.name.entireSize()
      + data.icon.entireSize()
      + (data.description ? data.description->entireSize() : 0u)
      + std::accumulate(data.spells.begin(), data.spells.end(), 0u,
                        [](auto a, const auto &b) {
                          return a + b.entireSize();
                        });
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::BSGN &t, std::size_t /*size*/) {
  os << t.editorID;
  os << t.name;
  os << t.icon;
  if (t.description) os << *t.description;
  for (const auto &spell : t.spells) os << spell;

  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::BSGN &t, std::size_t /*size*/) {
  readRecord(is, t.editorID, "EDID");
  readRecord(is, t.name, "NAME");
  readRecord(is, t.icon, "ICON");
  readRecord(is, t.description, "DESC");
  while (peekRecordType(is) == "SPLO") {
    record::SPLO r{};
    is >> r;
    t.spells.push_back(r);
  }
  return is;
}

// MISC specialization
template<>
uint32_t MISC::size() const {
  return data.editorID.entireSize()
      + (data.name ? data.name->entireSize() : 0u)
      + (data.modelFilename ? data.modelFilename->entireSize() : 0u)
      + (data.boundRadius ? data.boundRadius->entireSize() : 0u)
      + (data.textureHash ? data.textureHash->entireSize() : 0u)
      + (data.itemScript ? data.itemScript->entireSize() : 0u)
      + (data.icon ? data.icon->entireSize() : 0u)
      + data.data.entireSize();
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::MISC &t, std::size_t /*size*/) {
  os << t.editorID;
  if (t.name) os << *t.name;
  if (t.modelFilename) os << *t.modelFilename;
  if (t.boundRadius) os << *t.boundRadius;
  if (t.textureHash) os << *t.textureHash;
  if (t.icon) os << *t.icon;
  if (t.itemScript) os << *t.itemScript;
  os << t.data;

  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::MISC &t, std::size_t /*size*/) {
  readRecord(is, t.editorID, "EDID");
  readRecord(is, t.name, "FULL");
  readRecord(is, t.modelFilename, "MODL");
  readRecord(is, t.boundRadius, "MODB");
  readRecord(is, t.textureHash, "MODT");
  readRecord(is, t.icon, "ICON");
  readRecord(is, t.itemScript, "SCRI");
  readRecord(is, t.data, "DATA");

  return is;
}

// DOOR specialization
template<>
uint32_t DOOR::size() const {
  return data.editorID.entireSize()
      + (data.name ? data.name->entireSize() : 0u)
      + (data.modelFilename ? data.modelFilename->entireSize() : 0u)
      + (data.boundRadius ? data.boundRadius->entireSize() : 0u)
      + (data.textureHash ? data.textureHash->entireSize() : 0u)
      + (data.script ? data.script->entireSize() : 0u)
      + (data.openSound ? data.openSound->entireSize() : 0u)
      + (data.closeSound ? data.closeSound->entireSize() : 0u)
      + (data.loopSound ? data.loopSound->entireSize() : 0u)
      + data.flags.entireSize()
      + std::accumulate(data.randomTeleports.begin(),
                        data.randomTeleports.end(),
                        0u,
                        [](auto a, const auto &b) {
                          return a + b.entireSize();
                        });
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::DOOR &t, std::size_t /*size*/) {
  os << t.editorID;
  if (t.name) os << *t.name;
  if (t.modelFilename) os << *t.modelFilename;
  if (t.boundRadius) os << *t.boundRadius;
  if (t.textureHash) os << *t.textureHash;
  if (t.script) os << *t.script;
  if (t.openSound) os << *t.openSound;
  if (t.closeSound) os << *t.closeSound;
  if (t.loopSound) os << *t.loopSound;
  os << t.flags;
  for (const auto &rec : t.randomTeleports) os << rec;

  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::DOOR &t, std::size_t /*size*/) {
  readRecord(is, t.editorID, "EDID");
  readRecord(is, t.name, "FULL");
  readRecord(is, t.modelFilename, "MODL");
  readRecord(is, t.boundRadius, "MODB");
  readRecord(is, t.textureHash, "MODT");
  readRecord(is, t.script, "SCRI");
  readRecord(is, t.openSound, "SNAM");
  readRecord(is, t.closeSound, "ANAM");
  readRecord(is, t.loopSound, "BNAM");
  readRecord(is, t.flags, "FNAM");
  while (peekRecordType(is) == "TNAM") {
    record::TNAM_DOOR r{};
    is >> r;
    t.randomTeleports.push_back(r);
  }

  return is;
}

} // namespace record
