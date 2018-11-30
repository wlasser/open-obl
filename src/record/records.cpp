#include "io/io.hpp"
#include "record/record.hpp"
#include "record/records.hpp"
#include "record/subrecords.hpp"
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
  return (editorId ? editorId->entireSize() : 0u)
      + itemName.entireSize()
      + modelFilename.entireSize()
      + (boundRadius ? boundRadius->entireSize() : 0u)
      + (textureHash ? textureHash->entireSize() : 0u)
      + (iconFilename ? iconFilename->entireSize() : 0u)
      + (itemScript ? itemScript->entireSize() : 0u)
      + itemWeight.entireSize()
      + itemValue.entireSize()
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
  uint32_t size = header.entireSize()
      + (offsets ? offsets->entireSize() : 0u)
      + (deleted ? deleted->entireSize() : 0u)
      + (author ? author->entireSize() : 0u)
      + (description ? description->entireSize() : 0u);
  for (const auto &master : masters) {
    size += master.master.entireSize() + master.fileSize.entireSize();
  }
  return size;
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
  return editorId.entireSize() + value.entireSize();
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
  return editorId.entireSize() + type.entireSize()
      + value.entireSize();
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
  return editorId.entireSize() + name.entireSize()
      + description.entireSize() + iconFilename.entireSize()
      + data.entireSize();
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
  uint32_t size = editorId.entireSize() + name.entireSize()
      + flags.entireSize() + crimeGoldMultiplier.entireSize();
  for (const auto &r : relations) {
    size += r.entireSize();
  }
  for (const auto &r : ranks) {
    size += r.index.entireSize() + r.maleName.entireSize()
        + r.femaleName.entireSize() + r.iconFilename.entireSize();
  }
  return size;
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
  return editorId.entireSize() + name.entireSize()
      + modelFilename.entireSize() + boundRadius.entireSize()
      + boundRadius.entireSize() + textureHash.entireSize()
      + iconFilename.entireSize() + flags.entireSize();
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
  return editorId.entireSize() + name.entireSize()
      + iconFilename.entireSize() + flags.entireSize();
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
  uint32_t size = editorId.entireSize()
      + (name ? name->entireSize() : 0u)
      + description.entireSize();
  for (const auto &power : powers) size += power.entireSize();
  for (const auto &relation : relations) size += relation.entireSize();
  size += data.entireSize();
  if (voices) size += voices.value().entireSize();
  if (defaultHair) size += defaultHair.value().entireSize();
  size += defaultHairColor.entireSize()
      + (facegenMainClamp ? facegenMainClamp->entireSize() : 0u)
      + (facegenFaceClamp ? facegenFaceClamp->entireSize() : 0u)
      + baseAttributes.entireSize() + faceMarker.entireSize();
  for (const auto &faceData : faceData) {
    size += faceData.type.entireSize()
        + (faceData.modelFilename ? faceData.modelFilename->entireSize() : 0u)
        + (faceData.boundRadius ? faceData.boundRadius->entireSize() : 0u)
        + (faceData.textureFilename ? faceData.textureFilename->entireSize()
                                    : 0u);
  }
  size += bodyMarker.entireSize() + maleBodyMarker.entireSize();
  if (maleTailModel) {
    size += maleTailModel->model.entireSize()
        + maleTailModel->boundRadius.entireSize();
  }
  for (const auto &bodyData : maleBodyData) {
    size += bodyData.type.entireSize();
    if (bodyData.textureFilename) {
      size += bodyData.textureFilename.value().entireSize();
    }
  }
  size += femaleBodyMarker.entireSize();
  if (femaleTailModel) {
    size += femaleTailModel->model.entireSize()
        + femaleTailModel->boundRadius.entireSize();
  }
  for (const auto &bodyData : femaleBodyData) {
    size += bodyData.type.entireSize();
    if (bodyData.textureFilename) {
      size += bodyData.textureFilename.value().entireSize();
    }
  }
  size += hair.entireSize() + eyes.entireSize()
      + fggs.entireSize() + fgga.entireSize() + fgts.entireSize()
      + unused.entireSize();
  return size;
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
  return editorId.entireSize() + filename.entireSize()
      + std::visit([](auto &&r) {
        using T = std::decay_t<decltype(r)>;
        if constexpr (std::is_same_v<T, record::SNDD>) {
          return r.entireSize();
        } else if constexpr (std::is_same_v<T, record::SNDX>) {
          return r.entireSize();
        } else {
          return 0;
        }
      }, sound);
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
  return editorId.entireSize() + index.entireSize()
      + description.entireSize()
      + (iconFilename ? iconFilename->entireSize() : 0u)
      + data.entireSize() + apprenticeText.entireSize()
      + journeymanText.entireSize() + expertText.entireSize()
      + expertText.entireSize();
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
  return editorId.entireSize() + effectName.entireSize()
      + description.entireSize()
      + (iconFilename ? iconFilename->entireSize() : 0u)
      + (effectModel ? effectModel->entireSize() : 0u)
      + (boundRadius ? boundRadius->entireSize() : 0u)
      + data.entireSize() + counterEffects.entireSize();
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
  uint32_t size = editorId.entireSize() + textureFilename.entireSize()
      + (havokData ? havokData->entireSize() : 0u)
      + (specularExponent ? specularExponent->entireSize() : 0u);
  for (const auto &grass : potentialGrasses) {
    size += grass.entireSize();
  }
  return size;
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
  return editorId.entireSize() + modelFilename.entireSize()
      + boundRadius.entireSize()
      + (textureHash ? textureHash->entireSize() : 0u);
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
  return editorId.entireSize()
      + (name ? name->entireSize() : 0u)
      + enchantmentData.entireSize()
      + std::accumulate(effects.begin(), effects.end(), 0,
                        [](auto a, const auto &b) {
                          return a + b.size();
                        });
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
  return editorId.entireSize()
      + name.entireSize()
      + data.entireSize()
      + std::accumulate(effects.begin(), effects.end(), 0u,
                        [](auto a, const auto &b) {
                          return a + b.size();
                        });
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
  return editorId.entireSize()
      + (name ? name->entireSize() : 0u)
      + data.entireSize()
      + (lighting ? lighting->entireSize() : 0u)
      + (music ? music->entireSize() : 0u)
      + (owner ? owner->entireSize() : 0u)
      + (ownershipGlobal ? ownershipGlobal->entireSize() : 0u)
      + (ownershipRank ? ownershipRank->entireSize() : 0u)
      + (climate ? climate->entireSize() : 0u)
      + (water ? water->entireSize() : 0u)
      + (waterHeight ? waterHeight->entireSize() : 0u)
      + (regions ? regions->entireSize() : 0u)
      + (grid ? grid->entireSize() : 0u);
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

// LIGH specialization
template<> uint32_t LIGH::size() const {
  return editorId.entireSize()
      + (modelFilename ? modelFilename->entireSize() : 0u)
      + (boundRadius ? boundRadius->entireSize() : 0u)
      + (textureHash ? textureHash->entireSize() : 0u)
      + (itemScript ? itemScript->entireSize() : 0u)
      + (name ? name->entireSize() : 0u)
      + (icon ? icon->entireSize() : 0u)
      + data.entireSize()
      + (fadeValue ? fadeValue->entireSize() : 0u)
      + (sound ? sound->entireSize() : 0u);
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
  return editorId.entireSize()
      + name.entireSize()
      + icon.entireSize()
      + (description ? description->entireSize() : 0u)
      + std::accumulate(spells.begin(), spells.end(), 0u,
                        [](auto a, const auto &b) {
                          return a + b.entireSize();
                        });
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
  return editorId.entireSize()
      + (name ? name->entireSize() : 0u)
      + (modelFilename ? modelFilename->entireSize() : 0u)
      + (boundRadius ? boundRadius->entireSize() : 0u)
      + (textureHash ? textureHash->entireSize() : 0u)
      + (itemScript ? itemScript->entireSize() : 0u)
      + (icon ? icon->entireSize() : 0u)
      + data.entireSize();
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
  return editorId.entireSize()
      + (name ? name->entireSize() : 0u)
      + (modelFilename ? modelFilename->entireSize() : 0u)
      + (boundRadius ? boundRadius->entireSize() : 0u)
      + (textureHash ? textureHash->entireSize() : 0u)
      + (script ? script->entireSize() : 0u)
      + (openSound ? openSound->entireSize() : 0u)
      + (closeSound ? closeSound->entireSize() : 0u)
      + (loopSound ? loopSound->entireSize() : 0u)
      + flags.entireSize()
      + std::accumulate(randomTeleports.begin(),
                        randomTeleports.end(),
                        0u,
                        [](auto a, const auto &b) {
                          return a + b.entireSize();
                        });
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
  return editorId.entireSize()
      + (name ? name->entireSize() : 0u)
      + (modelFilename ? modelFilename->entireSize() : 0u)
      + (boundRadius ? boundRadius->entireSize() : 0u)
      + (textureHash ? textureHash->entireSize() : 0u)
      + (script ? script->entireSize() : 0u)
      + (sound ? sound->entireSize() : 0u);
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

BaseId peekBaseOfReference(std::istream &is) {
  const auto start{is.tellg()};
  raw::REFRBase refr{};
  readRecordHeader(is);
  refr.read(is);
  is.seekg(start, std::ios::beg);
  return refr.baseId.data;
}

} // namespace record
