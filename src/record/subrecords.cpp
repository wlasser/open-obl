#include "io/io.hpp"
#include "record/io.hpp"
#include "record/subrecords.hpp"
#include <gsl/gsl_util>
#include <istream>
#include <ostream>

namespace record::raw {

//===----------------------------------------------------------------------===//
// ACBS Specialization
//===----------------------------------------------------------------------===//
std::size_t SubrecordSize<ACBS>::operator()(const ACBS &) const { return 16u; }

void
SizedBinaryIo<ACBS>::writeBytes(std::ostream &os, const ACBS &t, std::size_t) {
  io::writeBytes(os, t.flags);
  io::writeBytes(os, t.baseSpellPoints);
  io::writeBytes(os, t.baseFatigue);
  io::writeBytes(os, t.barterGold);
  io::writeBytes(os, t.level);
  io::writeBytes(os, t.calcMin);
  io::writeBytes(os, t.calcMax);
}

void SizedBinaryIo<ACBS>::readBytes(std::istream &is, ACBS &t, std::size_t) {
  io::readBytes(is, t.flags);
  io::readBytes(is, t.baseSpellPoints);
  io::readBytes(is, t.baseFatigue);
  io::readBytes(is, t.barterGold);
  io::readBytes(is, t.level);
  io::readBytes(is, t.calcMin);
  io::readBytes(is, t.calcMax);
}

//===----------------------------------------------------------------------===//
// AIDT Specialization
//===----------------------------------------------------------------------===//
std::size_t SubrecordSize<AIDT>::operator()(const AIDT &) const { return 12u; }

void
SizedBinaryIo<AIDT>::writeBytes(std::ostream &os, const AIDT &t, std::size_t) {
  io::writeBytes(os, t.aggression);
  io::writeBytes(os, t.confidence);
  io::writeBytes(os, t.energyLevel);
  io::writeBytes(os, t.responsibility);
  io::writeBytes(os, t.flags);
  io::writeBytes(os, t.trainingSkill);
  io::writeBytes(os, t.trainingLevel);
  io::writeBytes(os, t.unknown);
}

void SizedBinaryIo<AIDT>::readBytes(std::istream &is, AIDT &t, std::size_t) {
  io::readBytes(is, t.aggression);
  io::readBytes(is, t.confidence);
  io::readBytes(is, t.energyLevel);
  io::readBytes(is, t.responsibility);
  io::readBytes(is, t.flags);
  io::readBytes(is, t.trainingSkill);
  io::readBytes(is, t.trainingLevel);
  io::readBytes(is, t.unknown);
}

//===----------------------------------------------------------------------===//
// DATA_CLAS Specialization
//===----------------------------------------------------------------------===//
std::size_t SubrecordSize<DATA_CLAS>::operator()(const DATA_CLAS &data) const {
  return 4u * 12u + (data.hasTrainingInfo ? 4u : 0u);
}

void SizedBinaryIo<DATA_CLAS>::writeBytes(std::ostream &os, const DATA_CLAS &t,
                                          std::size_t) {
  io::writeBytes(os, t.primaryAttributes);
  io::writeBytes(os, t.specialization);
  io::writeBytes(os, t.majorSkills);
  io::writeBytes(os, t.playableFlag);
  io::writeBytes(os, t.barterFlag);
  if (t.hasTrainingInfo) {
    io::writeBytes(os, t.skillTrained);
    io::writeBytes(os, t.maxTrainingLevel);
    io::writeBytes(os, t.unused);
  }
}

void SizedBinaryIo<DATA_CLAS>::readBytes(std::istream &is, DATA_CLAS &t,
                                         std::size_t size) {
  io::readBytes(is, t.primaryAttributes);
  io::readBytes(is, t.specialization);
  io::readBytes(is, t.majorSkills);
  io::readBytes(is, t.playableFlag);
  io::readBytes(is, t.barterFlag);
  if (size == 0x34) {
    t.hasTrainingInfo = true;
    io::readBytes(is, t.skillTrained);
    io::readBytes(is, t.maxTrainingLevel);
    io::readBytes(is, t.unused);
  }
}

//===----------------------------------------------------------------------===//
// DATA_CONT Specialization
//===----------------------------------------------------------------------===//
std::size_t SubrecordSize<DATA_CONT>::operator()(const DATA_CONT &) const {
  return 5u;
}

void SizedBinaryIo<DATA_CONT>::writeBytes(std::ostream &os, const DATA_CONT &t,
                                          std::size_t) {
  io::writeBytes(os, t.flag);
  io::writeBytes(os, t.weight);
}

void SizedBinaryIo<DATA_CONT>::readBytes(std::istream &is, DATA_CONT &t,
                                         std::size_t) {
  io::readBytes(is, t.flag);
  io::readBytes(is, t.weight);
}

//===----------------------------------------------------------------------===//
// DATA_GMST Specialization
//===----------------------------------------------------------------------===//
std::size_t SubrecordSize<DATA_GMST>::operator()(const DATA_GMST &data) const {
  return data.s.size();
}

void SizedBinaryIo<DATA_GMST>::writeBytes(std::ostream &os, const DATA_GMST &t,
                                          std::size_t) {
  os.write(t.s.data(), t.s.size() * 1);
}

void SizedBinaryIo<DATA_GMST>::readBytes(std::istream &is, DATA_GMST &t,
                                         std::size_t size) {
  t.s.clear();
  io::readBytes(is, t.s, size);
  if (size == 4) {
    t.i = *reinterpret_cast<int32_t *>(t.s.data());
    t.f = *reinterpret_cast<float *>(t.s.data());
  }
}

//===----------------------------------------------------------------------===//
// DATA_GRAS Specialization
//===----------------------------------------------------------------------===//
std::size_t SubrecordSize<DATA_GRAS>::operator()(const DATA_GRAS &) const {
  return 32u;
}

void SizedBinaryIo<DATA_GRAS>::writeBytes(std::ostream &os, const DATA_GRAS &t,
                                          std::size_t) {
  io::writeBytes(os, t.density);
  io::writeBytes(os, t.minSlope);
  io::writeBytes(os, t.maxSlope);
  io::writeBytes(os, t.unused1);
  io::writeBytes(os, t.unitsFromWater);
  io::writeBytes(os, t.unused2);
  io::writeBytes(os, t.unitsFromWaterType);
  io::writeBytes(os, t.positionRange);
  io::writeBytes(os, t.heightRange);
  io::writeBytes(os, t.colorRange);
  io::writeBytes(os, t.wavePeriod);
  io::writeBytes(os, t.flags);
}

void SizedBinaryIo<DATA_GRAS>::readBytes(std::istream &is, DATA_GRAS &t,
                                         std::size_t) {
  io::readBytes(is, t.density);
  io::readBytes(is, t.minSlope);
  io::readBytes(is, t.maxSlope);
  io::readBytes(is, t.unused1);
  io::readBytes(is, t.unitsFromWater);
  io::readBytes(is, t.unused2);
  io::readBytes(is, t.unitsFromWaterType);
  io::readBytes(is, t.positionRange);
  io::readBytes(is, t.heightRange);
  io::readBytes(is, t.colorRange);
  io::readBytes(is, t.wavePeriod);
  io::readBytes(is, t.flags);
}

//===----------------------------------------------------------------------===//
// DATA_LIGH Specialization
//===----------------------------------------------------------------------===//
std::size_t SubrecordSize<DATA_LIGH>::operator()(const DATA_LIGH &) const {
  return 32u;
}

void SizedBinaryIo<DATA_LIGH>::writeBytes(std::ostream &os, const DATA_LIGH &t,
                                          std::size_t) {
  io::writeBytes(os, t.time);
  io::writeBytes(os, t.radius);
  io::writeBytes(os, t.color);
  io::writeBytes(os, t.flags);
  io::writeBytes(os, t.falloffExponent);
  io::writeBytes(os, t.fov);
  io::writeBytes(os, t.value);
  io::writeBytes(os, t.weight);
}

void SizedBinaryIo<DATA_LIGH>::readBytes(std::istream &is, DATA_LIGH &t,
                                         std::size_t size) {
  io::readBytes(is, t.time);
  io::readBytes(is, t.radius);
  io::readBytes(is, t.color);
  io::readBytes(is, t.flags);
  if (size == 32) {
    io::readBytes(is, t.falloffExponent);
    io::readBytes(is, t.fov);
  }
  io::readBytes(is, t.value);
  io::readBytes(is, t.weight);
}

//===----------------------------------------------------------------------===//
// DATA_MGEF Specialization
//===----------------------------------------------------------------------===//
std::size_t SubrecordSize<DATA_MGEF>::operator()(const DATA_MGEF &) const {
  return sizeof(DATA_MGEF::Flag) + 4u + sizeof(oo::MagicSchool)
      + sizeof(oo::ActorValue) + 2u * sizeof(uint16_t) + 4u * sizeof(float)
      + 7u * sizeof(oo::FormId);
}

void SizedBinaryIo<DATA_MGEF>::writeBytes(std::ostream &os, const DATA_MGEF &t,
                                          std::size_t) {
  io::writeBytes(os, t.flags);
  io::writeBytes(os, t.baseCost);
  std::visit([&os](auto &&v) { io::writeBytes(os, v); }, t.associatedObject);
  io::writeBytes(os, t.school);
  io::writeBytes(os, t.resist);
  io::writeBytes(os, t.esceLength);
  io::writeBytes(os, t.unused);
  io::writeBytes(os, t.light);
  io::writeBytes(os, t.projectileSpeed);
  io::writeBytes(os, t.effectShader);
  io::writeBytes(os, t.enchantEffect);
  io::writeBytes(os, t.castingSound);
  io::writeBytes(os, t.boltSound);
  io::writeBytes(os, t.hitSound);
  io::writeBytes(os, t.areaSound);
  io::writeBytes(os, t.constantEffectEnchantmentFactor);
  io::writeBytes(os, t.constantEffectBarterFactor);
}

void SizedBinaryIo<DATA_MGEF>::readBytes(std::istream &is, DATA_MGEF &t,
                                         std::size_t size) {
  io::readBytes(is, t.flags);
  io::readBytes(is, t.baseCost);
  // TODO: This should not be a variant since we cannot deduce the argument
  //       type, but it cannot be a union for that reason either.
  std::visit([&is](auto &&v) { io::readBytes(is, v); }, t.associatedObject);
  io::readBytes(is, t.school);
  io::readBytes(is, t.resist);
  io::readBytes(is, t.esceLength);
  io::readBytes(is, t.unused);
  io::readBytes(is, t.light);
  io::readBytes(is, t.projectileSpeed);
  io::readBytes(is, t.effectShader);
  // Blame the Darkness spell for this.
  if (size == 0x24) return;
  io::readBytes(is, t.enchantEffect);
  io::readBytes(is, t.castingSound);
  io::readBytes(is, t.boltSound);
  io::readBytes(is, t.hitSound);
  io::readBytes(is, t.areaSound);
  io::readBytes(is, t.constantEffectEnchantmentFactor);
  io::readBytes(is, t.constantEffectBarterFactor);
}

//===----------------------------------------------------------------------===//
// DATA_RACE Specialization
//===----------------------------------------------------------------------===//
std::size_t SubrecordSize<DATA_RACE>::operator()(const DATA_RACE &data) const {
  return sizeof(oo::ActorValue) * 1u * data.skillModifiers.size() + 2u * 1u
      + 4u * 4u + 4u;
}

void SizedBinaryIo<DATA_RACE>::writeBytes(std::ostream &os, const DATA_RACE &t,
                                          std::size_t) {
  for (const auto &pair : t.skillModifiers) {
    // Despite being an oo::ActorValue, only the first byte is used
    auto av = static_cast<uint8_t>(pair.first);
    io::writeBytes(os, av);
    io::writeBytes(os, pair.second);
  }
  io::writeBytes(os, t.unused);
  io::writeBytes(os, t.heightMale);
  io::writeBytes(os, t.heightFemale);
  io::writeBytes(os, t.heightFemale);
  io::writeBytes(os, t.weightMale);
  io::writeBytes(os, t.weightFemale);
  io::writeBytes(os, t.flags);
}

void SizedBinaryIo<DATA_RACE>::readBytes(std::istream &is, DATA_RACE &t,
                                         std::size_t size) {
  // There can be zero to seven skill modifiers and they are not delimited, so
  // we have to compute how many there are using the total size.
  const auto allModifiersSize = size - 22u;
  const auto singleModifierSize = 2u;
  if (allModifiersSize % singleModifierSize != 0u) {
    throw std::range_error(
        std::string("Could not determine the number of skill modifiers: ")
            .append(std::to_string(allModifiersSize))
            .append(" is not divisible by ")
            .append(std::to_string(singleModifierSize)));
  }
  const auto numModifiers = allModifiersSize / singleModifierSize;
  for (auto i = 0u; i < numModifiers; ++i) {
    std::pair<uint8_t, int8_t> p{};
    io::readBytes(is, p);
    t.skillModifiers.emplace_back(static_cast<oo::ActorValue>(p.first),
                                  p.second);
  }
  io::readBytes(is, t.unused);
  io::readBytes(is, t.heightMale);
  io::readBytes(is, t.heightFemale);
  io::readBytes(is, t.weightMale);
  io::readBytes(is, t.weightFemale);
  io::readBytes(is, t.flags);
}

//===----------------------------------------------------------------------===//
// DATA_WTHR Specialization
//===----------------------------------------------------------------------===//
std::size_t SubrecordSize<DATA_WTHR>::operator()(const DATA_WTHR &) const {
  return 15u;
}

void SizedBinaryIo<DATA_WTHR>::writeBytes(std::ostream &os, const DATA_WTHR &t,
                                          std::size_t) {
  io::writeBytes(os, t.windSpeed);
  io::writeBytes(os, t.cloudSpeedLower);
  io::writeBytes(os, t.cloudSpeedUpper);
  io::writeBytes(os, t.transDelta);
  io::writeBytes(os, t.sunGlare);
  io::writeBytes(os, t.sunDamage);
  io::writeBytes(os, t.beginPrecipitationFadeIn);
  io::writeBytes(os, t.endPrecipitationFadeOut);
  io::writeBytes(os, t.beginThunderFadeIn);
  io::writeBytes(os, t.endThunderFadeOut);
  io::writeBytes(os, t.frequency);
  io::writeBytes(os, t.classification);
  io::writeBytes(os, t.lightningR);
  io::writeBytes(os, t.lightningG);
  io::writeBytes(os, t.lightningB);
}

void SizedBinaryIo<DATA_WTHR>::readBytes(std::istream &is, DATA_WTHR &t,
                                         std::size_t) {
  io::readBytes(is, t.windSpeed);
  io::readBytes(is, t.cloudSpeedLower);
  io::readBytes(is, t.cloudSpeedUpper);
  io::readBytes(is, t.transDelta);
  io::readBytes(is, t.sunGlare);
  io::readBytes(is, t.sunDamage);
  io::readBytes(is, t.beginPrecipitationFadeIn);
  io::readBytes(is, t.endPrecipitationFadeOut);
  io::readBytes(is, t.beginThunderFadeIn);
  io::readBytes(is, t.endThunderFadeOut);
  io::readBytes(is, t.frequency);
  io::readBytes(is, t.classification);
  io::readBytes(is, t.lightningR);
  io::readBytes(is, t.lightningG);
  io::readBytes(is, t.lightningB);
}

//===----------------------------------------------------------------------===//
// DELE Specialization
//===----------------------------------------------------------------------===//
std::size_t SubrecordSize<DELE>::operator()(const DELE &data) const {
  return data.size;
}

void SizedBinaryIo<DELE>::writeBytes(std::ostream &os, const DELE &,
                                     std::size_t size) {
  for (std::size_t i = 0u; i < size; ++i) os.put('\0');
}

void
SizedBinaryIo<DELE>::readBytes(std::istream &is, DELE &t, std::size_t size) {
  is.seekg(size, std::istream::cur);
  t.size = gsl::narrow_cast<uint32_t>(size);
}

//===----------------------------------------------------------------------===//
// EFIT Specialization
//===----------------------------------------------------------------------===//
std::size_t SubrecordSize<EFIT>::operator()(const EFIT &) const {
  return 5u * 4u + sizeof(oo::ActorValue);
}

void
SizedBinaryIo<EFIT>::writeBytes(std::ostream &os, const EFIT &t, std::size_t) {
  io::writeBytes(os, t.efid);
  io::writeBytes(os, t.magnitude);
  io::writeBytes(os, t.area);
  io::writeBytes(os, t.duration);
  io::writeBytes(os, t.type);
  io::writeBytes(os, t.avIndex);
}

void SizedBinaryIo<EFIT>::readBytes(std::istream &is, EFIT &t, std::size_t) {
  io::readBytes(is, t.efid);
  io::readBytes(is, t.magnitude);
  io::readBytes(is, t.area);
  io::readBytes(is, t.duration);
  io::readBytes(is, t.type);
  io::readBytes(is, t.avIndex);
}

//===----------------------------------------------------------------------===//
// ENAM Specialization
//===----------------------------------------------------------------------===//
std::size_t SubrecordSize<ENAM>::operator()(const ENAM &data) const {
  return data.eyes.size() * sizeof(oo::FormId);
}

void
SizedBinaryIo<ENAM>::writeBytes(std::ostream &os, const ENAM &t, std::size_t) {
  io::writeBytes(os, t.eyes);
}

void
SizedBinaryIo<ENAM>::readBytes(std::istream &is, ENAM &t, std::size_t size) {
  io::readBytes(is, t.eyes, size / sizeof(oo::FormId));
}

//===----------------------------------------------------------------------===//
// ENIT Specialization
//===----------------------------------------------------------------------===//
std::size_t SubrecordSize<ENIT>::operator()(const ENIT &) const { return 8u; }

void SizedBinaryIo<ENIT>::writeBytes(std::ostream &os, const ENIT &t,
                                     std::size_t) {
  io::writeBytes(os, t.value);
  io::writeBytes(os, t.flags);
  io::writeBytes(os, t.unused);
}

void SizedBinaryIo<ENIT>::readBytes(std::istream &is, ENIT &t, std::size_t) {
  io::readBytes(is, t.value);
  io::readBytes(is, t.flags);
  io::readBytes(is, t.unused);
}

//===----------------------------------------------------------------------===//
// ENIT_ENCH Specialization
//===----------------------------------------------------------------------===//
std::size_t SubrecordSize<ENIT_ENCH>::operator()(const ENIT_ENCH &) const {
  return 3u * 4u + 1u + 3u * 1u;
}

void SizedBinaryIo<ENIT_ENCH>::writeBytes(std::ostream &os, const ENIT_ENCH &t,
                                          std::size_t) {
  io::writeBytes(os, t.type);
  io::writeBytes(os, t.chargeAmount);
  io::writeBytes(os, t.chargeCost);
  io::writeBytes(os, t.noAutoCalculate);
  io::writeBytes(os, t.unused);
}

void SizedBinaryIo<ENIT_ENCH>::readBytes(std::istream &is, ENIT_ENCH &t,
                                         std::size_t) {
  io::readBytes(is, t.type);
  io::readBytes(is, t.chargeAmount);
  io::readBytes(is, t.chargeCost);
  io::readBytes(is, t.noAutoCalculate);
  io::readBytes(is, t.unused);
}

//===----------------------------------------------------------------------===//
// ESCE Specialization
//===----------------------------------------------------------------------===//
std::size_t SubrecordSize<ESCE>::operator()(const ESCE &data) const {
  return data.effects.size() * sizeof(oo::EffectId);
}

void
SizedBinaryIo<ESCE>::writeBytes(std::ostream &os, const ESCE &t, std::size_t) {
  io::writeBytes(os, t.effects);
}

void
SizedBinaryIo<ESCE>::readBytes(std::istream &is, ESCE &t, std::size_t size) {
  io::readBytes(is, t.effects, size / sizeof(oo::EffectId));
}

//===----------------------------------------------------------------------===//
// HNAM Specialization
//===----------------------------------------------------------------------===//
std::size_t SubrecordSize<HNAM>::operator()(const HNAM &data) const {
  return data.hair.size() * sizeof(oo::FormId);
}

void
SizedBinaryIo<HNAM>::writeBytes(std::ostream &os, const HNAM &t, std::size_t) {
  io::writeBytes(os, t.hair);
}

void
SizedBinaryIo<HNAM>::readBytes(std::istream &is, HNAM &t, std::size_t size) {
  io::readBytes(is, t.hair, size / sizeof(oo::FormId));
}

//===----------------------------------------------------------------------===//
// HNAM_LTEX Specialization
//===----------------------------------------------------------------------===//
std::size_t SubrecordSize<HNAM_LTEX>::operator()(const HNAM_LTEX &) const {
  return 3u * 1u;
}

void SizedBinaryIo<HNAM_LTEX>::writeBytes(std::ostream &os, const HNAM_LTEX &t,
                                          std::size_t) {
  io::writeBytes(os, t.type);
  io::writeBytes(os, t.friction);
  io::writeBytes(os, t.restitution);
}

void SizedBinaryIo<HNAM_LTEX>::readBytes(std::istream &is, HNAM_LTEX &t,
                                         std::size_t) {
  io::readBytes(is, t.type);
  io::readBytes(is, t.friction);
  io::readBytes(is, t.restitution);
}

//===----------------------------------------------------------------------===//
// MNAM_WRLD Specialization
//===----------------------------------------------------------------------===//
std::size_t SubrecordSize<MNAM_WRLD>::operator()(const MNAM_WRLD &) const {
  return 16u;
}

void SizedBinaryIo<MNAM_WRLD>::writeBytes(std::ostream &os, const MNAM_WRLD &t,
                                          std::size_t) {
  io::writeBytes(os, t.width);
  io::writeBytes(os, t.height);
  io::writeBytes(os, t.topLeft.x);
  io::writeBytes(os, t.topLeft.y);
  io::writeBytes(os, t.bottomRight.x);
  io::writeBytes(os, t.bottomRight.y);
}

void SizedBinaryIo<MNAM_WRLD>::readBytes(std::istream &is, MNAM_WRLD &t,
                                         std::size_t) {
  io::readBytes(is, t.width);
  io::readBytes(is, t.height);
  io::readBytes(is, t.topLeft.x);
  io::readBytes(is, t.topLeft.y);
  io::readBytes(is, t.bottomRight.x);
  io::readBytes(is, t.bottomRight.y);
}

//===----------------------------------------------------------------------===//
// MODT Specialization
//===----------------------------------------------------------------------===//
std::size_t SubrecordSize<MODT>::operator()(const MODT &data) const {
  return 3u * 8u * data.records.size();
}

void
SizedBinaryIo<MODT>::writeBytes(std::ostream &os, const MODT &t, std::size_t) {
  io::writeBytes(os, t.records);
}

void
SizedBinaryIo<MODT>::readBytes(std::istream &is, MODT &t, std::size_t size) {
  io::readBytes(is, t.records, size / sizeof(MODT::MODTRecord));
}

//===----------------------------------------------------------------------===//
// NAM0_WTHR Specialization
//===----------------------------------------------------------------------===//
std::size_t SubrecordSize<NAM0_WTHR>::operator()(const NAM0_WTHR &) const {
  return 10u * 4u * 4u;
}

void SizedBinaryIo<NAM0_WTHR>::writeBytes(std::ostream &os, const NAM0_WTHR &t,
                                          std::size_t) {
  io::writeBytes(os, t.skyUpper);
  io::writeBytes(os, t.fog);
  io::writeBytes(os, t.cloudsLower);
  io::writeBytes(os, t.ambient);
  io::writeBytes(os, t.sunlight);
  io::writeBytes(os, t.sun);
  io::writeBytes(os, t.stars);
  io::writeBytes(os, t.skyLower);
  io::writeBytes(os, t.horizon);
  io::writeBytes(os, t.cloudsUpper);
}

void SizedBinaryIo<NAM0_WTHR>::readBytes(std::istream &is, NAM0_WTHR &t,
                                         std::size_t) {
  io::readBytes(is, t.skyUpper);
  io::readBytes(is, t.fog);
  io::readBytes(is, t.cloudsLower);
  io::readBytes(is, t.ambient);
  io::readBytes(is, t.sunlight);
  io::readBytes(is, t.sun);
  io::readBytes(is, t.stars);
  io::readBytes(is, t.skyLower);
  io::readBytes(is, t.horizon);
  io::readBytes(is, t.cloudsUpper);
}

//===----------------------------------------------------------------------===//
// OFST Specialization
//===----------------------------------------------------------------------===//
std::size_t SubrecordSize<OFST>::operator()(const OFST &data) const {
  return 3u * 4u * data.unused.size();
}

void
SizedBinaryIo<OFST>::writeBytes(std::ostream &os, const OFST &t, std::size_t) {
  io::writeBytes(os, t.unused);
}

void
SizedBinaryIo<OFST>::readBytes(std::istream &is, OFST &t, std::size_t size) {
  io::readBytes(is, t.unused, size / 12u);
}

//===----------------------------------------------------------------------===//
// OFST_WRLD Specialization
//===----------------------------------------------------------------------===//
std::size_t SubrecordSize<OFST_WRLD>::operator()(const OFST_WRLD &data) const {
  return data.entries.size() * 4u;
}

void SizedBinaryIo<OFST_WRLD>::writeBytes(std::ostream &os, const OFST_WRLD &t,
                                          std::size_t) {
  io::writeBytes(os, t.entries);
}

void SizedBinaryIo<OFST_WRLD>::readBytes(std::istream &is, OFST_WRLD &t,
                                         std::size_t size) {
  io::readBytes(is, t.entries, size / 4u);
}

//===----------------------------------------------------------------------===//
// SCIT Specialization
//===----------------------------------------------------------------------===//
std::size_t SubrecordSize<SCIT>::operator()(const SCIT &) const {
  return sizeof(oo::FormId) + sizeof(oo::MagicSchool) + 8u;
}

void
SizedBinaryIo<SCIT>::writeBytes(std::ostream &os, const SCIT &t, std::size_t) {
  io::writeBytes(os, t.id);
  io::writeBytes(os, t.school);
  io::writeBytes(os, t.visualEffect);
  io::writeBytes(os, t.flags);
  io::writeBytes(os, t.unused);
}

void SizedBinaryIo<SCIT>::readBytes(std::istream &is, SCIT &t, std::size_t) {
  io::readBytes(is, t.id);
  io::readBytes(is, t.school);
  io::readBytes(is, t.visualEffect);
  io::readBytes(is, t.flags);
  io::readBytes(is, t.unused);
}

//===----------------------------------------------------------------------===//
// SNAM_TREE Specialization
//===----------------------------------------------------------------------===//
std::size_t SubrecordSize<SNAM_TREE>::operator()(const SNAM_TREE &data) const {
  return 4u * data.seeds.size();
}

void SizedBinaryIo<SNAM_TREE>::writeBytes(std::ostream &os, const SNAM_TREE &t,
                                          std::size_t) {
  io::writeBytes(os, t.seeds);
}

void SizedBinaryIo<SNAM_TREE>::readBytes(std::istream &is, SNAM_TREE &t,
                                         std::size_t size) {
  io::readBytes(is, t.seeds, size / 4u);
}

//===----------------------------------------------------------------------===//
// SNAM_WTHR Specialization
//===----------------------------------------------------------------------===//
std::size_t SubrecordSize<SNAM_WTHR>::operator()(const SNAM_WTHR &) const {
  return 8u;
}

void SizedBinaryIo<SNAM_WTHR>::writeBytes(std::ostream &os, const SNAM_WTHR &t,
                                          std::size_t) {
  io::writeBytes(os, t.soundId);
  io::writeBytes(os, t.soundType);
}

void SizedBinaryIo<SNAM_WTHR>::readBytes(std::istream &is, SNAM_WTHR &t,
                                         std::size_t) {
  io::readBytes(is, t.soundId);
  io::readBytes(is, t.soundType);
}

//===----------------------------------------------------------------------===//
// SNDD Specialization
//===----------------------------------------------------------------------===//
std::size_t SubrecordSize<SNDD>::operator()(const SNDD &) const {
  return 4u * 1u + 4u;
}

void
SizedBinaryIo<SNDD>::writeBytes(std::ostream &os, const SNDD &t, std::size_t) {
  io::writeBytes(os, t.minAttenuationDistance);
  io::writeBytes(os, t.maxAttenuationDistance);
  io::writeBytes(os, t.frequencyAdjustment);
  io::writeBytes(os, t.unused);
  io::writeBytes(os, t.flags);
}

void
SizedBinaryIo<SNDD>::readBytes(std::istream &is, SNDD &t, std::size_t) {
  io::readBytes(is, t.minAttenuationDistance);
  io::readBytes(is, t.maxAttenuationDistance);
  io::readBytes(is, t.frequencyAdjustment);
  io::readBytes(is, t.unused);
  io::readBytes(is, t.flags);
}

//===----------------------------------------------------------------------===//
// SNDX Specialization
//===----------------------------------------------------------------------===//
std::size_t SubrecordSize<SNDX>::operator()(const SNDX &data) const {
  return 4u * 1u + 2u * 4u + (data.staticAttenuation ? 4u : 0u)
      + (data.startTime ? 4u : 0u) + (data.stopTime ? 4u : 0u);
}

void
SizedBinaryIo<SNDX>::writeBytes(std::ostream &os, const SNDX &t, std::size_t) {
  io::writeBytes(os, t.minAttenuationDistance);
  io::writeBytes(os, t.maxAttenuationDistance);
  io::writeBytes(os, t.frequencyAdjustment);
  io::writeBytes(os, t.unused);
  io::writeBytes(os, t.flags);
  io::writeBytes(os, t.unusedWord);
  io::writeBytes(os, t.staticAttenuation);
  io::writeBytes(os, t.startTime);
  io::writeBytes(os, t.stopTime);
}

void
SizedBinaryIo<SNDX>::readBytes(std::istream &is, SNDX &t, std::size_t size) {
  io::readBytes(is, t.minAttenuationDistance);
  io::readBytes(is, t.maxAttenuationDistance);
  io::readBytes(is, t.frequencyAdjustment);
  io::readBytes(is, t.unused);
  io::readBytes(is, t.flags);
  io::readBytes(is, t.unusedWord);
  if (size > 12) io::readBytes(is, t.staticAttenuation);
  if (size > 16) io::readBytes(is, t.startTime);
  if (size > 17) io::readBytes(is, t.stopTime);
}

//===----------------------------------------------------------------------===//
// SPIT Specialization
//===----------------------------------------------------------------------===//
std::size_t SubrecordSize<SPIT>::operator()(const SPIT &) const { return 16u; }

void
SizedBinaryIo<SPIT>::writeBytes(std::ostream &os, const SPIT &t, std::size_t) {
  io::writeBytes(os, t.type);
  io::writeBytes(os, t.cost);
  io::writeBytes(os, t.level);
  io::writeBytes(os, t.flags);
}

void
SizedBinaryIo<SPIT>::readBytes(std::istream &is, SPIT &t, std::size_t) {
  io::readBytes(is, t.type);
  io::readBytes(is, t.cost);
  io::readBytes(is, t.level);
  io::readBytes(is, t.flags);
}

//===----------------------------------------------------------------------===//
// TNAM_CLMT Specialization
//===----------------------------------------------------------------------===//
std::size_t SubrecordSize<TNAM_CLMT>::operator()(const TNAM_CLMT &) const {
  return 6u;
}

void SizedBinaryIo<TNAM_CLMT>::writeBytes(std::ostream &os, const TNAM_CLMT &t,
                                          std::size_t) {
  io::writeBytes(os, t.sunriseBegin);
  io::writeBytes(os, t.sunriseEnd);
  io::writeBytes(os, t.sunsetBegin);
  io::writeBytes(os, t.sunsetEnd);
  io::writeBytes(os, t.volatility);

  // Possibly surprising type conversions: consider
  // static_cast<uint32_t>(t.hasMasser) << 7u
  // static_cast<uint8_t>(t.hasMasser) << 7u
  // The first expression is unsigned, the second is *signed*.
  auto flag{(static_cast<uint32_t>(t.hasMasser) << 7u)
                & (static_cast<uint32_t>(t.hasSecunda) << 6u)
                & t.phaseLength};
  io::writeBytes(os, static_cast<uint8_t>(flag));
}

void SizedBinaryIo<TNAM_CLMT>::readBytes(std::istream &is, TNAM_CLMT &t,
                                         std::size_t) {
  io::readBytes(is, t.sunriseBegin);
  io::readBytes(is, t.sunriseEnd);
  io::readBytes(is, t.sunsetBegin);
  io::readBytes(is, t.sunsetEnd);
  io::readBytes(is, t.volatility);

  uint8_t flag;
  io::readBytes(is, flag);
  t.hasMasser = flag >> 7u;
  t.hasSecunda = (flag & 0b01000000u) >> 6u;
  t.phaseLength = flag & 0b00111111u;
}

//===----------------------------------------------------------------------===//
// VTXT Specialization
//===----------------------------------------------------------------------===//
std::size_t SubrecordSize<VTXT>::operator()(const VTXT &data) const {
  return data.points.size() * 8u;
}

void
SizedBinaryIo<VTXT>::writeBytes(std::ostream &os, const VTXT &t, std::size_t) {
  io::writeBytes(os, t.points);
}

void
SizedBinaryIo<VTXT>::readBytes(std::istream &is, VTXT &t, std::size_t size) {
  io::readBytes(is, t.points, size / 8u);
}

//===----------------------------------------------------------------------===//
// WLST Specialization
//===----------------------------------------------------------------------===//
std::size_t SubrecordSize<WLST>::operator()(const WLST &data) const {
  return data.weathers.size() * 8u;
}

void
SizedBinaryIo<WLST>::writeBytes(std::ostream &os, const WLST &t, std::size_t) {
  io::writeBytes(os, t.weathers);
}

void
SizedBinaryIo<WLST>::readBytes(std::istream &is, WLST &t, std::size_t size) {
  io::readBytes(is, t.weathers, size / 8u);
}

//===----------------------------------------------------------------------===//
// XCLR Specialization
//===----------------------------------------------------------------------===//
std::size_t SubrecordSize<XCLR>::operator()(const XCLR &data) const {
  return sizeof(oo::FormId) * data.regions.size();
}

void
SizedBinaryIo<XCLR>::writeBytes(std::ostream &os, const XCLR &t, std::size_t) {
  io::writeBytes(os, t.regions);
}

void
SizedBinaryIo<XCLR>::readBytes(std::istream &is, XCLR &t, std::size_t size) {
  io::readBytes(is, t.regions, size / 4u);
}

//===----------------------------------------------------------------------===//
// XESP Specialization
//===----------------------------------------------------------------------===//
std::size_t SubrecordSize<XESP>::operator()(const XESP &) const {
  return sizeof(oo::FormId) + sizeof(XESP::Flag);
}

void
SizedBinaryIo<XESP>::writeBytes(std::ostream &os, const XESP &t, std::size_t) {
  io::writeBytes(os, t.parent);
  io::writeBytes(os, t.flags);
}

void
SizedBinaryIo<XESP>::readBytes(std::istream &is, XESP &t, std::size_t) {
  io::readBytes(is, t.parent);
  io::readBytes(is, t.flags);
}

//===-----------------------------------------------------------------------==//
// XLOC Specialization
//===-----------------------------------------------------------------------==//
std::size_t SubrecordSize<XLOC>::operator()(const XLOC &) const {
  return 4u + sizeof(oo::FormId) + 4u;
}

void
SizedBinaryIo<XLOC>::writeBytes(std::ostream &os, const XLOC &t, std::size_t) {
  io::writeBytes(os, t.lockLevel);
  io::writeBytes(os, t.key);
  //io::writeBytes(os, t.unused);
  io::writeBytes(os, t.flags);
}

void
SizedBinaryIo<XLOC>::readBytes(std::istream &is, XLOC &t, std::size_t size) {
  io::readBytes(is, t.lockLevel);
  io::readBytes(is, t.key);
  if (size == 16) io::readBytes(is, t.unused);
  io::readBytes(is, t.flags);
}

//===----------------------------------------------------------------------===//
// XRGD Specialization
//===----------------------------------------------------------------------===//
std::size_t SubrecordSize<XRGD>::operator()(const XRGD &data) const {
  return data.bytes.size() * 1u;
}

void
SizedBinaryIo<XRGD>::writeBytes(std::ostream &os, const XRGD &t, std::size_t) {
  io::writeBytes(os, t.bytes);
}

void
SizedBinaryIo<XRGD>::readBytes(std::istream &is, XRGD &t, std::size_t size) {
  io::readBytes(is, t.bytes, size / 1u);
}

//===----------------------------------------------------------------------===//
// XSED Specialization
//===----------------------------------------------------------------------===//
std::size_t SubrecordSize<XSED>::operator()(const XSED &data) const {
  return data.size;
}

void SizedBinaryIo<XSED>::writeBytes(std::ostream &os, const XSED &,
                                     std::size_t size) {
  for (std::size_t i = 0u; i < size; ++i) os.put(0);
}

void
SizedBinaryIo<XSED>::readBytes(std::istream &is, XSED &t, std::size_t size) {
  t.size = gsl::narrow_cast<uint16_t>(size);
  is.seekg(size, std::ios_base::cur);
}

} // namespace record::raw;
