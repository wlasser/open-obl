#include "io/io.hpp"
#include "record/io.hpp"
#include "record/records.hpp"
#include <istream>
#include <numeric>
#include <ostream>
#include <set>

namespace record {

using namespace io;

//===----------------------------------------------------------------------===//
// Effect members
//===----------------------------------------------------------------------===//
namespace raw {

std::size_t raw::Effect::size() const {
  return name.entireSize() + data.entireSize()
      + (script ? (script->data.entireSize() + script->name.entireSize()) : 0u);
}

void raw::Effect::read(std::istream &is) {
  readRecord(is, name);
  readRecord(is, data);
  if (peekRecordType(is) == "SCIT"_rec) {
    ScriptEffectData sed{};
    readRecord(is, sed.data);
    readRecord(is, sed.name);
    script.emplace(std::move(sed));
  }
}

void raw::Effect::write(std::ostream &os) const {
  writeRecord(os, name);
  writeRecord(os, data);
  if (script) {
    writeRecord(os, script->data);
    writeRecord(os, script->name);
  }
}

bool raw::Effect::isNext(std::istream &is) {
  return peekRecordType(is) == "EFID"_rec;
}

} // namespace raw

//===----------------------------------------------------------------------===//
// TES4 Specialization
//===----------------------------------------------------------------------===//
template<> std::size_t TES4::size() const {
  return SizeOf(header)
      + SizeOf(offsets)
      + SizeOf(deleted)
      + SizeOf(author)
      + SizeOf(description)
      + std::accumulate(masters.begin(), masters.end(), std::size_t{0u},
                        [](std::size_t a, const auto &b) {
                          return a + SizeOf(b.master) + SizeOf(b.fileSize);
                        });
}

void raw::SizedBinaryIo<raw::TES4>::writeBytes(
    std::ostream &os, const raw::TES4 &t, std::size_t) {
  writeRecord(os, t.header);
  writeRecord(os, t.offsets);
  writeRecord(os, t.author);
  writeRecord(os, t.description);
  for (const auto &entry : t.masters) {
    writeRecord(os, entry.master);
    writeRecord(os, entry.fileSize);
  }
}

void raw::SizedBinaryIo<raw::TES4>::readBytes(
    std::istream &is, raw::TES4 &t, std::size_t) {
  readRecord(is, t.header);
  readRecord(is, t.offsets);
  readRecord(is, t.deleted);
  readRecord(is, t.author);
  readRecord(is, t.description);

  while (peekRecordType(is) == "MAST"_rec) {
    raw::TES4::Master master{};
    readRecord(is, master.master);
    readRecord(is, master.fileSize);
    t.masters.emplace_back(std::move(master));
  }
}

//===----------------------------------------------------------------------===//
// GMST Specialization
//===----------------------------------------------------------------------===//
template<> std::size_t GMST::size() const {
  return SizeOf(editorId) + SizeOf(value);
}

void raw::SizedBinaryIo<raw::GMST>::writeBytes(
    std::ostream &os, const raw::GMST &t, std::size_t) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.value);
}

void raw::SizedBinaryIo<raw::GMST>::readBytes(
    std::istream &is, raw::GMST &t, std::size_t) {
  readRecord(is, t.editorId);
  readRecord(is, t.value);
}

//===----------------------------------------------------------------------===//
// GLOB Specialization
//===----------------------------------------------------------------------===//
template<> std::size_t GLOB::size() const {
  return SizeOf(editorId) + SizeOf(type) + SizeOf(value);
}

void raw::SizedBinaryIo<raw::GLOB>::writeBytes(
    std::ostream &os, const raw::GLOB &t, std::size_t) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.type);
  writeRecord(os, t.value);
}

void raw::SizedBinaryIo<raw::GLOB>::readBytes(
    std::istream &is, raw::GLOB &t, std::size_t) {
  readRecord(is, t.editorId);
  readRecord(is, t.type);
  readRecord(is, t.value);
}

//===----------------------------------------------------------------------===//
// CLAS Specialization
//===----------------------------------------------------------------------===//
template<> std::size_t CLAS::size() const {
  return SizeOf(editorId)
      + SizeOf(name)
      + SizeOf(description)
      + SizeOf(iconFilename)
      + SizeOf(data);
}

void raw::SizedBinaryIo<raw::CLAS>::writeBytes(
    std::ostream &os, const raw::CLAS &t, std::size_t) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.name);
  writeRecord(os, t.description);
  writeRecord(os, t.iconFilename);
  writeRecord(os, t.data);
}

void raw::SizedBinaryIo<raw::CLAS>::readBytes(
    std::istream &is, raw::CLAS &t, std::size_t) {
  readRecord(is, t.editorId);
  readRecord(is, t.name);
  readRecord(is, t.description);
  readRecord(is, t.iconFilename);
  readRecord(is, t.data);
}

//===----------------------------------------------------------------------===//
// FACT Specialization
//===----------------------------------------------------------------------===//
template<> std::size_t FACT::size() const {
  return SizeOf(editorId)
      + SizeOf(name)
      + SizeOf(relations)
      + SizeOf(crimeGoldMultiplier)
      + std::accumulate(ranks.begin(), ranks.end(), std::size_t{0u},
                        [](std::size_t a, const auto &b) {
                          return a
                              + SizeOf(b.index)
                              + SizeOf(b.maleName)
                              + SizeOf(b.femaleName)
                              + SizeOf(b.iconFilename);
                        });
}

void raw::SizedBinaryIo<raw::FACT>::writeBytes(
    std::ostream &os, const raw::FACT &t, std::size_t) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.name);
  writeRecord(os, t.relations);
  writeRecord(os, t.flags);
  writeRecord(os, t.crimeGoldMultiplier);
  for (const auto &r : t.ranks) {
    writeRecord(os, r.index);
    writeRecord(os, r.maleName);
    writeRecord(os, r.femaleName);
    writeRecord(os, r.iconFilename);
  }
}

void raw::SizedBinaryIo<raw::FACT>::readBytes(
    std::istream &is, raw::FACT &t, std::size_t) {
  readRecord(is, t.editorId);
  readRecord(is, t.name);
  readRecord(is, t.relations);
  readRecord(is, t.flags);
  readRecord(is, t.crimeGoldMultiplier);
  while (peekRecordType(is) == "RNAM"_rec) {
    raw::FACT::Rank r{};
    readRecord(is, r.index);
    readRecord(is, r.maleName);
    readRecord(is, r.femaleName);
    readRecord(is, r.iconFilename);
    t.ranks.emplace_back(std::move(r));
  }
}

//===----------------------------------------------------------------------===//
// HAIR Specialization
//===----------------------------------------------------------------------===//
template<> std::size_t HAIR::size() const {
  return SizeOf(editorId)
      + SizeOf(name)
      + SizeOf(modelFilename)
      + SizeOf(boundRadius)
      + SizeOf(textureHash)
      + SizeOf(iconFilename)
      + SizeOf(flags);
}

void raw::SizedBinaryIo<raw::HAIR>::writeBytes(
    std::ostream &os, const raw::HAIR &t, std::size_t) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.name);
  writeRecord(os, t.modelFilename);
  writeRecord(os, t.boundRadius);
  writeRecord(os, t.textureHash);
  writeRecord(os, t.iconFilename);
  writeRecord(os, t.flags);
}

void raw::SizedBinaryIo<raw::HAIR>::readBytes(
    std::istream &is, raw::HAIR &t, std::size_t) {
  readRecord(is, t.editorId);
  readRecord(is, t.name);
  readRecord(is, t.modelFilename);
  readRecord(is, t.boundRadius);
  readRecord(is, t.textureHash);
  readRecord(is, t.iconFilename);
  readRecord(is, t.flags);
}

//===----------------------------------------------------------------------===//
// EYES Specialization
//===----------------------------------------------------------------------===//
template<> std::size_t EYES::size() const {
  return SizeOf(editorId) + SizeOf(name) + SizeOf(iconFilename) + SizeOf(flags);
}

void raw::SizedBinaryIo<raw::EYES>::writeBytes(
    std::ostream &os, const raw::EYES &t, std::size_t) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.name);
  writeRecord(os, t.iconFilename);
  writeRecord(os, t.flags);
}

void raw::SizedBinaryIo<raw::EYES>::readBytes(
    std::istream &is, raw::EYES &t, std::size_t) {
  readRecord(is, t.editorId);
  readRecord(is, t.name);
  readRecord(is, t.iconFilename);
  readRecord(is, t.flags);
}

//===----------------------------------------------------------------------===//
// RACE Specialization
//===----------------------------------------------------------------------===//
template<> std::size_t RACE::size() const {
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
      + std::accumulate(faceData.begin(), faceData.end(), std::size_t{0u},
                        [](std::size_t a, const auto &b) {
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
      + std::accumulate(maleBodyData.begin(), maleBodyData.end(),
                        std::size_t{0u}, [](std::size_t a, const auto &b) {
            return a + SizeOf(b.type) + SizeOf(b.textureFilename);
          })
      + SizeOf(femaleBodyMarker)
      + [&t = femaleTailModel]() {
        return t ? (SizeOf(t->model) + SizeOf(t->boundRadius)) : 0u;
      }()
      + std::accumulate(femaleBodyData.begin(), femaleBodyData.end(),
                        std::size_t{0u}, [](std::size_t a, const auto &b) {
            return a + SizeOf(b.type) + SizeOf(b.textureFilename);
          })
      + SizeOf(hair)
      + SizeOf(eyes)
      + SizeOf(fggs)
      + SizeOf(fgga)
      + SizeOf(fgts)
      + SizeOf(unused);
}

void raw::SizedBinaryIo<raw::RACE>::writeBytes(
    std::ostream &os, const raw::RACE &t, std::size_t) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.name);
  writeRecord(os, t.description);
  writeRecord(os, t.powers);
  writeRecord(os, t.relations);
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
}

void raw::SizedBinaryIo<raw::RACE>::readBytes(
    std::istream &is, raw::RACE &t, std::size_t) {
  readRecord(is, t.editorId);
  readRecord(is, t.name);
  readRecord(is, t.description);

  readRecord(is, t.powers);
  readRecord(is, t.relations);

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
    readRecord(is, f.type);
    readRecord(is, f.modelFilename);
    readRecord(is, f.boundRadius);
    readRecord(is, f.textureFilename);
    t.faceData.emplace_back(std::move(f));
  }

  readRecord(is, t.bodyMarker);
  readRecord(is, t.maleBodyMarker);

  if (peekRecordType(is) == "MODL"_rec) {
    raw::RACE::TailData tail{};
    readRecord(is, tail.model);
    readRecord(is, tail.boundRadius);
    t.maleTailModel.emplace(std::move(tail));
  } else {
    t.maleTailModel = std::nullopt;
  }

  while (peekRecordType(is) == "INDX"_rec) {
    raw::RACE::BodyData b{};
    readRecord(is, b.type);
    readRecord(is, b.textureFilename);
    t.maleBodyData.emplace_back(std::move(b));
  }

  readRecord(is, t.femaleBodyMarker);

  if (peekRecordType(is) == "MODL"_rec) {
    raw::RACE::TailData tail{};
    readRecord(is, tail.model);
    readRecord(is, tail.boundRadius);
    t.femaleTailModel.emplace(std::move(tail));
  } else {
    t.femaleTailModel = std::nullopt;
  }

  while (peekRecordType(is) == "INDX"_rec) {
    raw::RACE::BodyData b{};
    readRecord(is, b.type);
    readRecord(is, b.textureFilename);
    t.femaleBodyData.emplace_back(std::move(b));
  }

  readRecord(is, t.hair);
  readRecord(is, t.eyes);
  readRecord(is, t.fggs);
  readRecord(is, t.fgga);
  readRecord(is, t.fgts);
  readRecord(is, t.unused);
}

//===----------------------------------------------------------------------===//
// SOUN Specialization
//===----------------------------------------------------------------------===//
template<> std::size_t SOUN::size() const {
  return SizeOf(editorId)
      + SizeOf(filename)
      + std::visit([](const auto &r) { return SizeOf(r); }, sound);
}

void raw::SizedBinaryIo<raw::SOUN>::writeBytes(
    std::ostream &os, const raw::SOUN &t, std::size_t) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.filename);
  std::visit([&os](const auto &r) { writeRecord(os, r); }, t.sound);
}

void raw::SizedBinaryIo<raw::SOUN>::readBytes(
    std::istream &is, raw::SOUN &t, std::size_t) {
  readRecord(is, t.editorId);
  readRecord(is, t.filename);

  if (peekRecordType(is) == "SNDD"_rec) {
    t.sound.emplace<0>(readRecord<record::SNDD>(is));
  } else if (peekRecordType(is) == "SNDX"_rec) {
    t.sound.emplace<1>(readRecord<record::SNDX>(is));
  }
}

//===----------------------------------------------------------------------===//
// SKIL Specialization
//===----------------------------------------------------------------------===//
template<> std::size_t SKIL::size() const {
  return SizeOf(editorId)
      + SizeOf(index)
      + SizeOf(iconFilename)
      + SizeOf(data)
      + SizeOf(apprenticeText)
      + SizeOf(journeymanText)
      + SizeOf(expertText)
      + SizeOf(masterText);
}

void raw::SizedBinaryIo<raw::SKIL>::writeBytes(
    std::ostream &os, const raw::SKIL &t, std::size_t) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.index);
  writeRecord(os, t.description);
  writeRecord(os, t.iconFilename);
  writeRecord(os, t.data);
  writeRecord(os, t.apprenticeText);
  writeRecord(os, t.journeymanText);
  writeRecord(os, t.expertText);
  writeRecord(os, t.masterText);
}

void raw::SizedBinaryIo<raw::SKIL>::readBytes(
    std::istream &is, raw::SKIL &t, std::size_t) {
  readRecord(is, t.editorId);
  readRecord(is, t.index);
  readRecord(is, t.description);
  readRecord(is, t.iconFilename);
  readRecord(is, t.data);
  readRecord(is, t.apprenticeText);
  readRecord(is, t.journeymanText);
  readRecord(is, t.expertText);
  readRecord(is, t.masterText);
}

//===----------------------------------------------------------------------===//
// MGEF Specialization
//===----------------------------------------------------------------------===//
template<> std::size_t MGEF::size() const {
  return SizeOf(editorId)
      + SizeOf(effectName)
      + SizeOf(description)
      + SizeOf(iconFilename)
      + SizeOf(effectModel)
      + SizeOf(boundRadius)
      + SizeOf(data)
      + SizeOf(counterEffects);
}

void raw::SizedBinaryIo<raw::MGEF>::writeBytes(
    std::ostream &os, const raw::MGEF &t, std::size_t) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.effectName);
  writeRecord(os, t.description);
  writeRecord(os, t.iconFilename);
  writeRecord(os, t.effectModel);
  writeRecord(os, t.boundRadius);
  writeRecord(os, t.data);
  writeRecord(os, t.counterEffects);
}

void raw::SizedBinaryIo<raw::MGEF>::readBytes(
    std::istream &is, raw::MGEF &t, std::size_t) {
  readRecord(is, t.editorId);
  readRecord(is, t.effectName);
  readRecord(is, t.description);
  readRecord(is, t.iconFilename);
  readRecord(is, t.effectModel);
  readRecord(is, t.boundRadius);
  readRecord(is, t.data);
  readRecord(is, t.counterEffects);
}

//===----------------------------------------------------------------------===//
// LTEX Specialization
//===----------------------------------------------------------------------===//
template<> std::size_t LTEX::size() const {
  return SizeOf(editorId)
      + SizeOf(textureFilename)
      + SizeOf(havokData)
      + SizeOf(specularExponent)
      + SizeOf(potentialGrasses);
}

void raw::SizedBinaryIo<raw::LTEX>::writeBytes(
    std::ostream &os, const raw::LTEX &t, std::size_t) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.textureFilename);
  writeRecord(os, t.havokData);
  writeRecord(os, t.specularExponent);
  writeRecord(os, t.potentialGrasses);
}

void raw::SizedBinaryIo<raw::LTEX>::readBytes(
    std::istream &is, raw::LTEX &t, std::size_t) {
  readRecord(is, t.editorId);
  readRecord(is, t.textureFilename);
  readRecord(is, t.havokData);
  readRecord(is, t.specularExponent);
  readRecord(is, t.potentialGrasses);
}

//===----------------------------------------------------------------------===//
// ENCH Specialization
//===----------------------------------------------------------------------===//
template<> std::size_t ENCH::size() const {
  return SizeOf(editorId)
      + SizeOf(name)
      + SizeOf(enchantmentData)
      + std::accumulate(effects.begin(), effects.end(), std::size_t{0u},
                        [](std::size_t a, const auto &b) {
                          return a + b.size();
                        });
}

void raw::SizedBinaryIo<raw::ENCH>::writeBytes(
    std::ostream &os, const raw::ENCH &t, std::size_t) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.name);
  writeRecord(os, t.enchantmentData);
  for (const auto &effect : t.effects) effect.write(os);
}

void raw::SizedBinaryIo<raw::ENCH>::readBytes(
    std::istream &is, raw::ENCH &t, std::size_t) {
  readRecord(is, t.editorId);
  readRecord(is, t.name);
  readRecord(is, t.enchantmentData);
  while (Effect::isNext(is)) t.effects.emplace_back().read(is);
}

//===----------------------------------------------------------------------===//
// SPEL Specialization
//===----------------------------------------------------------------------===//
template<> std::size_t SPEL::size() const {
  return SizeOf(editorId)
      + SizeOf(name)
      + SizeOf(data)
      + std::accumulate(effects.begin(), effects.end(), std::size_t{0u},
                        [](std::size_t a, const auto &b) {
                          return a + b.size();
                        });
}

void raw::SizedBinaryIo<raw::SPEL>::writeBytes(
    std::ostream &os, const raw::SPEL &t, std::size_t) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.name);
  writeRecord(os, t.data);
  for (const auto &effect : t.effects) effect.write(os);
}

void raw::SizedBinaryIo<raw::SPEL>::readBytes(
    std::istream &is, raw::SPEL &t, std::size_t) {
  readRecord(is, t.editorId);
  readRecord(is, t.name);
  readRecord(is, t.data);
  while (Effect::isNext(is)) t.effects.emplace_back().read(is);
}

//===----------------------------------------------------------------------===//
// BSGN Specialization
//===----------------------------------------------------------------------===//
template<> std::size_t BSGN::size() const {
  return SizeOf(editorId)
      + SizeOf(name)
      + SizeOf(icon)
      + SizeOf(description)
      + SizeOf(spells);
}

void raw::SizedBinaryIo<raw::BSGN>::writeBytes(
    std::ostream &os, const raw::BSGN &t, std::size_t) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.name);
  writeRecord(os, t.icon);
  writeRecord(os, t.description);
  writeRecord(os, t.spells);
}

void raw::SizedBinaryIo<raw::BSGN>::readBytes(
    std::istream &is, raw::BSGN &t, std::size_t) {
  readRecord(is, t.editorId);
  readRecord(is, t.name);
  readRecord(is, t.icon);
  readRecord(is, t.description);
  readRecord(is, t.spells);
}

//===----------------------------------------------------------------------===//
// ACTI Specialization
//===----------------------------------------------------------------------===//
template<> std::size_t ACTI::size() const {
  return SizeOf(editorId)
      + SizeOf(name)
      + SizeOf(modelFilename)
      + SizeOf(boundRadius)
      + SizeOf(textureHash)
      + SizeOf(script)
      + SizeOf(sound);
}

void raw::SizedBinaryIo<raw::ACTI>::writeBytes(
    std::ostream &os, const raw::ACTI &t, std::size_t) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.name);
  writeRecord(os, t.modelFilename);
  writeRecord(os, t.boundRadius);
  writeRecord(os, t.textureHash);
  writeRecord(os, t.script);
  writeRecord(os, t.sound);
}

void raw::SizedBinaryIo<raw::ACTI>::readBytes(
    std::istream &is, raw::ACTI &t, std::size_t) {
  readRecord(is, t.editorId);
  readRecord(is, t.name);
  readRecord(is, t.modelFilename);
  readRecord(is, t.boundRadius);
  readRecord(is, t.textureHash);
  readRecord(is, t.script);
  readRecord(is, t.sound);
}

//===----------------------------------------------------------------------===//
// CONT Specialization
//===----------------------------------------------------------------------===//
template<> std::size_t CONT::size() const {
  return SizeOf(editorId)
      + SizeOf(modelFilename)
      + SizeOf(boundRadius)
      + SizeOf(textureHash)
      + SizeOf(data)
      + SizeOf(openSound)
      + SizeOf(closeSound)
      + SizeOf(script)
      + SizeOf(items);
}

void raw::SizedBinaryIo<raw::CONT>::writeBytes(
    std::ostream &os, const raw::CONT &t, std::size_t) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.name);
  writeRecord(os, t.modelFilename);
  writeRecord(os, t.boundRadius);
  writeRecord(os, t.textureHash);
  writeRecord(os, t.script);
  writeRecord(os, t.items);
  writeRecord(os, t.data);
  writeRecord(os, t.openSound);
  writeRecord(os, t.closeSound);
}

void raw::SizedBinaryIo<raw::CONT>::readBytes(
    std::istream &is, raw::CONT &t, std::size_t) {
  readRecord(is, t.editorId);
  readRecord(is, t.name);
  readRecord(is, t.modelFilename);
  readRecord(is, t.boundRadius);
  readRecord(is, t.textureHash);
  readRecord(is, t.script);
  readRecord(is, t.items);
  readRecord(is, t.data);
  readRecord(is, t.openSound);
  readRecord(is, t.closeSound);
}

//===----------------------------------------------------------------------===//
// DOOR Specialization
//===----------------------------------------------------------------------===//
template<> std::size_t DOOR::size() const {
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

void raw::SizedBinaryIo<raw::DOOR>::writeBytes(
    std::ostream &os, const raw::DOOR &t, std::size_t) {
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
  writeRecord(os, t.randomTeleports);
}

void raw::SizedBinaryIo<raw::DOOR>::readBytes(
    std::istream &is, raw::DOOR &t, std::size_t) {
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
  readRecord(is, t.randomTeleports);
}

//===----------------------------------------------------------------------===//
// LIGH Specialization
//===----------------------------------------------------------------------===//
template<> std::size_t LIGH::size() const {
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

void raw::SizedBinaryIo<raw::LIGH>::writeBytes(
    std::ostream &os, const raw::LIGH &t, std::size_t) {
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
}

void raw::SizedBinaryIo<raw::LIGH>::readBytes(
    std::istream &is, raw::LIGH &t, std::size_t) {
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
}

//===----------------------------------------------------------------------===//
// MISC Specialization
//===----------------------------------------------------------------------===//
template<> std::size_t MISC::size() const {
  return SizeOf(editorId)
      + SizeOf(name)
      + SizeOf(modelFilename)
      + SizeOf(boundRadius)
      + SizeOf(textureHash)
      + SizeOf(itemScript)
      + SizeOf(icon)
      + SizeOf(data);
}

void raw::SizedBinaryIo<raw::MISC>::writeBytes(
    std::ostream &os, const raw::MISC &t, std::size_t) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.name);
  writeRecord(os, t.modelFilename);
  writeRecord(os, t.boundRadius);
  writeRecord(os, t.textureHash);
  writeRecord(os, t.icon);
  writeRecord(os, t.itemScript);
  writeRecord(os, t.data);
}

void raw::SizedBinaryIo<raw::MISC>::readBytes(
    std::istream &is, raw::MISC &t, std::size_t) {
  readRecord(is, t.editorId);
  readRecord(is, t.name);
  readRecord(is, t.modelFilename);
  readRecord(is, t.boundRadius);
  readRecord(is, t.textureHash);
  readRecord(is, t.icon);
  readRecord(is, t.itemScript);
  readRecord(is, t.data);
}

//===----------------------------------------------------------------------===//
// STAT Specialization
//===----------------------------------------------------------------------===//
template<> std::size_t STAT::size() const {
  return SizeOf(editorId)
      + SizeOf(modelFilename)
      + SizeOf(boundRadius)
      + SizeOf(textureHash);
}

void raw::SizedBinaryIo<raw::STAT>::writeBytes(
    std::ostream &os, const raw::STAT &t, std::size_t) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.modelFilename);
  writeRecord(os, t.boundRadius);
  writeRecord(os, t.textureHash);
}

void raw::SizedBinaryIo<raw::STAT>::readBytes(
    std::istream &is, raw::STAT &t, std::size_t size) {
  // There are a few corrupted records making this harder than it needs to be:
  // ARVineRising02 has no MODL, MODB, or MODT
  // Empty STAT (size 0) after PalaceDRug01
  if (size == 0) return;
  readRecord(is, t.editorId);
  if (peekRecordType(is) != "MODL"_rec) return;
  readRecord(is, t.modelFilename);
  readRecord(is, t.boundRadius);
  readRecord(is, t.textureHash);
}

//===----------------------------------------------------------------------===//
// GRAS Specialization
//===----------------------------------------------------------------------===//
template<> std::size_t GRAS::size() const {
  return SizeOf(editorId)
      + SizeOf(modelFilename)
      + SizeOf(boundRadius)
      + SizeOf(textureHash)
      + SizeOf(data);
}

void raw::SizedBinaryIo<raw::GRAS>::writeBytes(
    std::ostream &os, const raw::GRAS &t, std::size_t) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.modelFilename);
  writeRecord(os, t.boundRadius);
  writeRecord(os, t.textureHash);
  writeRecord(os, t.data);
}

void raw::SizedBinaryIo<raw::GRAS>::readBytes(
    std::istream &is, raw::GRAS &t, std::size_t) {
  readRecord(is, t.editorId);
  readRecord(is, t.modelFilename);
  readRecord(is, t.boundRadius);
  readRecord(is, t.textureHash);
  readRecord(is, t.data);
}

//===----------------------------------------------------------------------===//
// TREE Specialization
//===----------------------------------------------------------------------===//
template<> std::size_t TREE::size() const {
  return SizeOf(editorId)
      + SizeOf(modelFilename)
      + SizeOf(boundRadius)
      + SizeOf(textureHash)
      + SizeOf(leafFilename)
      + SizeOf(seeds)
      + SizeOf(data)
      + SizeOf(billboardDimensions);
}

void raw::SizedBinaryIo<raw::TREE>::writeBytes(
    std::ostream &os, const raw::TREE &t, std::size_t) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.modelFilename);
  writeRecord(os, t.boundRadius);
  writeRecord(os, t.textureHash);
  writeRecord(os, t.leafFilename);
  writeRecord(os, t.seeds);
  writeRecord(os, t.data);
  writeRecord(os, t.billboardDimensions);
}

void raw::SizedBinaryIo<raw::TREE>::readBytes(
    std::istream &is, raw::TREE &t, std::size_t) {
  readRecord(is, t.editorId);
  readRecord(is, t.modelFilename);
  readRecord(is, t.boundRadius);
  readRecord(is, t.textureHash);
  readRecord(is, t.leafFilename);
  readRecord(is, t.seeds);
  readRecord(is, t.data);
  readRecord(is, t.billboardDimensions);
}

//===----------------------------------------------------------------------===//
// FLOR Specialization
//===----------------------------------------------------------------------===//
template<> std::size_t FLOR::size() const {
  return SizeOf(editorId)
      + SizeOf(name)
      + SizeOf(modelFilename)
      + SizeOf(boundRadius)
      + SizeOf(textureHash)
      + SizeOf(script)
      + SizeOf(ingredient)
      + SizeOf(harvestChances);
}

void raw::SizedBinaryIo<raw::FLOR>::writeBytes(
    std::ostream &os, const raw::FLOR &t, std::size_t) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.name);
  writeRecord(os, t.modelFilename);
  writeRecord(os, t.boundRadius);
  writeRecord(os, t.textureHash);
  writeRecord(os, t.script);
  writeRecord(os, t.ingredient);
  writeRecord(os, t.harvestChances);
}

void raw::SizedBinaryIo<raw::FLOR>::readBytes(
    std::istream &is, raw::FLOR &t, std::size_t) {
  readRecord(is, t.editorId);
  readRecord(is, t.name);
  readRecord(is, t.modelFilename);
  readRecord(is, t.boundRadius);
  readRecord(is, t.textureHash);
  readRecord(is, t.script);
  readRecord(is, t.ingredient);
  readRecord(is, t.harvestChances);
}

//===----------------------------------------------------------------------===//
// FURN Specialization
//===----------------------------------------------------------------------===//
template<> std::size_t FURN::size() const {
  return SizeOf(editorId)
      + SizeOf(name)
      + SizeOf(modelFilename)
      + SizeOf(boundRadius)
      + SizeOf(textureHash)
      + SizeOf(script)
      + SizeOf(activeMarkers);
}

void raw::SizedBinaryIo<raw::FURN>::writeBytes(
    std::ostream &os, const raw::FURN &t, std::size_t) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.name);
  writeRecord(os, t.modelFilename);
  writeRecord(os, t.boundRadius);
  writeRecord(os, t.textureHash);
  writeRecord(os, t.script);
  writeRecord(os, t.activeMarkers);
}

void raw::SizedBinaryIo<raw::FURN>::readBytes(
    std::istream &is, raw::FURN &t, std::size_t) {
  readRecord(is, t.editorId);
  readRecord(is, t.name);
  readRecord(is, t.modelFilename);
  readRecord(is, t.boundRadius);
  readRecord(is, t.textureHash);
  readRecord(is, t.script);
  readRecord(is, t.activeMarkers);
}

//===----------------------------------------------------------------------===//
// NPC_ Specialization
//===----------------------------------------------------------------------===//
template<> std::size_t NPC_::size() const {
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

void raw::SizedBinaryIo<raw::NPC_>::writeBytes(
    std::ostream &os, const raw::NPC_ &t, std::size_t) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.name);
  writeRecord(os, t.skeletonFilename);
  writeRecord(os, t.boundRadius);
  writeRecord(os, t.baseConfig);
  writeRecord(os, t.factions);
  writeRecord(os, t.deathItem);
  writeRecord(os, t.race);
  writeRecord(os, t.spells);
  writeRecord(os, t.script);
  writeRecord(os, t.items);
  writeRecord(os, t.aiData);
  writeRecord(os, t.aiPackages);
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
}

void raw::SizedBinaryIo<raw::NPC_>::readBytes(
    std::istream &is, raw::NPC_ &t, std::size_t) {
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
}

//===----------------------------------------------------------------------===//
// ALCH Specialization
//===----------------------------------------------------------------------===//
template<> std::size_t ALCH::size() const {
  return SizeOf(editorId)
      + SizeOf(itemName)
      + SizeOf(modelFilename)
      + SizeOf(boundRadius)
      + SizeOf(textureHash)
      + SizeOf(iconFilename)
      + SizeOf(itemScript)
      + SizeOf(itemWeight)
      + SizeOf(itemValue)
      + std::accumulate(effects.begin(), effects.end(), std::size_t{0u},
                        [](std::size_t a, const auto &b) {
                          return a + b.size();
                        });
}

void raw::SizedBinaryIo<raw::ALCH>::writeBytes(
    std::ostream &os, const raw::ALCH &t, std::size_t) {
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
}

void raw::SizedBinaryIo<raw::ALCH>::readBytes(
    std::istream &is, raw::ALCH &t, std::size_t) {
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
}

//===----------------------------------------------------------------------===//
// WTHR Specialization
//===----------------------------------------------------------------------===//
template<> std::size_t WTHR::size() const {
  return SizeOf(editorId)
      + SizeOf(lowerLayerFilename)
      + SizeOf(upperLayerFilename)
      + SizeOf(precipitationFilename)
      + SizeOf(precipitationBoundRadius)
      + SizeOf(skyColors)
      + SizeOf(fogDistances)
      + SizeOf(hdr)
      + SizeOf(data)
      + SizeOf(sounds);
}

void raw::SizedBinaryIo<raw::WTHR>::writeBytes(
    std::ostream &os, const raw::WTHR &t, const std::size_t) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.lowerLayerFilename);
  writeRecord(os, t.upperLayerFilename);
  writeRecord(os, t.precipitationFilename);
  writeRecord(os, t.precipitationBoundRadius);
  writeRecord(os, t.skyColors);
  writeRecord(os, t.fogDistances);
  writeRecord(os, t.hdr);
  writeRecord(os, t.data);
  writeRecord(os, t.sounds);
}

void raw::SizedBinaryIo<raw::WTHR>::readBytes(
    std::istream &is, raw::WTHR &t, const std::size_t) {
  readRecord(is, t.editorId);
  readRecord(is, t.lowerLayerFilename);
  readRecord(is, t.upperLayerFilename);
  readRecord(is, t.precipitationFilename);
  readRecord(is, t.precipitationBoundRadius);
  readRecord(is, t.skyColors);
  readRecord(is, t.fogDistances);
  readRecord(is, t.hdr);
  readRecord(is, t.data);
  readRecord(is, t.sounds);
}

//===----------------------------------------------------------------------===//
// CLMT Specialization
//===----------------------------------------------------------------------===//
template<> std::size_t CLMT::size() const {
  return SizeOf(editorId)
      + SizeOf(weatherList)
      + SizeOf(sunFilename)
      + SizeOf(sunglareFilename)
      + SizeOf(skyFilename)
      + SizeOf(boundRadius)
      + SizeOf(settings);
}

void raw::SizedBinaryIo<raw::CLMT>::writeBytes(
    std::ostream &os, const raw::CLMT &t, std::size_t) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.weatherList);
  writeRecord(os, t.sunFilename);
  writeRecord(os, t.sunglareFilename);
  writeRecord(os, t.skyFilename);
  writeRecord(os, t.boundRadius);
  writeRecord(os, t.settings);
}

void raw::SizedBinaryIo<raw::CLMT>::readBytes(
    std::istream &is, raw::CLMT &t, std::size_t) {
  readRecord(is, t.editorId);
  readRecord(is, t.weatherList);
  readRecord(is, t.sunFilename);
  readRecord(is, t.sunglareFilename);
  readRecord(is, t.skyFilename);
  readRecord(is, t.boundRadius);
  readRecord(is, t.settings);
}

//===----------------------------------------------------------------------===//
// CELL Specialization
//===----------------------------------------------------------------------===//
template<> std::size_t CELL::size() const {
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

void raw::SizedBinaryIo<raw::CELL>::writeBytes(
    std::ostream &os, const raw::CELL &t, std::size_t) {
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
}

void raw::SizedBinaryIo<raw::CELL>::readBytes(
    std::istream &is, raw::CELL &t, std::size_t) {
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
      default: return;
    }
  }
}

//===----------------------------------------------------------------------===//
// WRLD Specialization
//===----------------------------------------------------------------------===//
template<> std::size_t WRLD::size() const {
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

void raw::SizedBinaryIo<raw::WRLD>::writeBytes(
    std::ostream &os, const raw::WRLD &t, std::size_t) {
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
}

void raw::SizedBinaryIo<raw::WRLD>::readBytes(
    std::istream &is, raw::WRLD &t, std::size_t) {
  readRecord(is, t.editorId);
  std::set<uint32_t> possibleSubrecords = {
      "FULL"_rec, "WNAM"_rec, "SNAM"_rec, "ICON"_rec, "CNAM"_rec,
      "NAM2"_rec, "MNAM"_rec, "DATA"_rec, "NAM0"_rec, "NAM9"_rec,
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
      case "DATA"_rec:readRecord(is, t.data);
        break;
      case "NAM0"_rec:readRecord(is, t.bottomLeft);
        break;
      case "NAM9"_rec:readRecord(is, t.topRight);
        break;
      default: break;
    }
  }

  // We don't care about the annoying OFST record, so skip over it.
  if (peekRecordType(is) == "XXXX"_rec) {
    const uint32_t ofstSize{readRecord<record::XXXX>(is).data};
    is.seekg(ofstSize + 6u, std::ios_base::cur);
  } else if (peekRecordType(is) == "OFST"_rec) {
    (void) readRecord<record::OFST_WRLD>(is);
  }
}

//===----------------------------------------------------------------------===//
// LAND Specialization
//===----------------------------------------------------------------------===//
template<> std::size_t LAND::size() const {
  return SizeOf(data)
      + SizeOf(normals)
      + SizeOf(heights)
      + SizeOf(colors)
      + SizeOf(quadrantTexture)
      + SizeOf(fineTextures)
      + SizeOf(coarseTextures);
}

void raw::SizedBinaryIo<raw::LAND>::writeBytes(
    std::ostream &os, const raw::LAND &t, std::size_t) {
  writeRecord(os, t.data);
  writeRecord(os, t.normals);
  writeRecord(os, t.heights);
  writeRecord(os, t.colors);
  writeRecord(os, t.quadrantTexture);
  for (const auto &[a, v] : t.fineTextures) {
    writeRecord(os, a);
    writeRecord(os, v);
  }
  writeRecord(os, t.coarseTextures);
}

void raw::SizedBinaryIo<raw::LAND>::readBytes(
    std::istream &is, raw::LAND &t, std::size_t) {
  readRecord(is, t.data);
  readRecord(is, t.normals);
  readRecord(is, t.heights);
  readRecord(is, t.colors);

  // Expect either VTEX or (BTXT (ATXT VTXT)+)+
  if (peekRecordType(is) == "VTEX"_rec) {
    readRecord(is, t.coarseTextures);
    return;
  }

  do {
    auto recType = peekRecordType(is);
    if (recType == "BTXT"_rec) {
      readRecord(is, t.quadrantTexture.emplace_back());
    } else if (recType == "ATXT"_rec) {
      // CheydinhalExterior15 has a blank ATXT record with no VTXT record that
      // we want to ignore. It happens so rarely it's easier to just read the
      // ATXT anyway then discard the record.
      auto &p{t.fineTextures.emplace_back()};
      readRecord(is, p.first);
      if (peekRecordType(is) == "VTXT"_rec) {
        readRecord(is, p.second);
      } else {
        t.fineTextures.pop_back();
      }
    } else {
      break;
    }
  } while (true);
}

//===----------------------------------------------------------------------===//
// WATR Specialization
//===----------------------------------------------------------------------===//
template<> std::size_t WATR::size() const {
  return SizeOf(editorId)
      + SizeOf(textureFilename)
      + SizeOf(opacity)
      + SizeOf(flags)
      + SizeOf(materialId)
      + SizeOf(soundId)
      + SizeOf(data)
      + SizeOf(variants);
}

void raw::SizedBinaryIo<raw::WATR>::writeBytes(
    std::ostream &os, const raw::WATR &t, std::size_t) {
  writeRecord(os, t.editorId);
  writeRecord(os, t.textureFilename);
  writeRecord(os, t.opacity);
  writeRecord(os, t.flags);
  writeRecord(os, t.materialId);
  writeRecord(os, t.soundId);
  writeRecord(os, t.data);
  writeRecord(os, t.variants);
}

void raw::SizedBinaryIo<raw::WATR>::readBytes(
    std::istream &is, raw::WATR &t, std::size_t) {
  readRecord(is, t.editorId);
  readRecord(is, t.textureFilename);
  readRecord(is, t.opacity);
  readRecord(is, t.flags);
  readRecord(is, t.materialId);
  readRecord(is, t.soundId);

  // There are lots of old and unused WATR records that have broken DATA_WATR
  // subrecords, presumably from previous stages of development. We can detect
  // these by checking the size of the DATA_WATR subrecord, which should be
  // constant. Really this should be done in the subrecord itself, but if we
  // do that then it won't be tuplifiable anymore, and I don't feel like writing
  // out the huge read and write functions.
  std::array<char, 4> type{};
  io::readBytes(is, type);
  uint16_t size{};
  io::readBytes(is, size);
  if (size == 0x66) {
    is.seekg(-6, std::ios_base::cur);
    readRecord(is, t.data);
  } else {
    is.seekg(size, std::ios_base::cur);
  }

  readRecord(is, t.variants);
}

//===----------------------------------------------------------------------===//
// REFR Specializations
//===----------------------------------------------------------------------===//
void raw::SizedBinaryIo<raw::REFR_ACTI>::writeBytes(
    std::ostream &os, const raw::REFR_ACTI &t, std::size_t) {
  t.write(os);
}

void raw::SizedBinaryIo<raw::REFR_ACTI>::readBytes(
    std::istream &is, raw::REFR_ACTI &t, std::size_t) {
  t.read(is);
}

void raw::SizedBinaryIo<raw::REFR_CONT>::writeBytes(
    std::ostream &os, const raw::REFR_CONT &t, std::size_t) {
  t.write(os);
}

void raw::SizedBinaryIo<raw::REFR_CONT>::readBytes(
    std::istream &is, raw::REFR_CONT &t, std::size_t) {
  t.read(is);
}

void raw::SizedBinaryIo<raw::REFR_DOOR>::writeBytes(
    std::ostream &os, const raw::REFR_DOOR &t, std::size_t) {
  t.write(os);
}

void raw::SizedBinaryIo<raw::REFR_DOOR>::readBytes(
    std::istream &is, raw::REFR_DOOR &t, std::size_t) {
  t.read(is);
}

void raw::SizedBinaryIo<raw::REFR_LIGH>::writeBytes(
    std::ostream &os, const raw::REFR_LIGH &t, std::size_t) {
  t.write(os);
}

void raw::SizedBinaryIo<raw::REFR_LIGH>::readBytes(
    std::istream &is, raw::REFR_LIGH &t, std::size_t) {
  t.read(is);
}

void raw::SizedBinaryIo<raw::REFR_MISC>::writeBytes(
    std::ostream &os, const raw::REFR_MISC &t, std::size_t) {
  t.write(os);
}

void raw::SizedBinaryIo<raw::REFR_MISC>::readBytes(
    std::istream &is, raw::REFR_MISC &t, std::size_t) {
  t.read(is);
}

void raw::SizedBinaryIo<raw::REFR_STAT>::writeBytes(
    std::ostream &os, const raw::REFR_STAT &t, std::size_t) {
  t.write(os);
}

void raw::SizedBinaryIo<raw::REFR_STAT>::readBytes(
    std::istream &is, raw::REFR_STAT &t, std::size_t) {
  t.read(is);
}

void raw::SizedBinaryIo<raw::REFR_FLOR>::writeBytes(
    std::ostream &os, const raw::REFR_FLOR &t, std::size_t) {
  t.write(os);
}

void raw::SizedBinaryIo<raw::REFR_FLOR>::readBytes(
    std::istream &is, raw::REFR_FLOR &t, std::size_t) {
  t.read(is);
}

void raw::SizedBinaryIo<raw::REFR_FURN>::writeBytes(
    std::ostream &os, const raw::REFR_FURN &t, std::size_t) {
  t.write(os);
}

void raw::SizedBinaryIo<raw::REFR_FURN>::readBytes(
    std::istream &is, raw::REFR_FURN &t, std::size_t) {
  t.read(is);
}

void raw::SizedBinaryIo<raw::REFR_NPC_>::writeBytes(
    std::ostream &os, const raw::REFR_NPC_ &t, std::size_t) {
  t.write(os);
}

void raw::SizedBinaryIo<raw::REFR_NPC_>::readBytes(
    std::istream &is, raw::REFR_NPC_ &t, std::size_t) {
  t.read(is);
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
