#include "io/io.hpp"
#include "record/io.hpp"
#include "record/subrecords.hpp"
#include <istream>
#include <ostream>

namespace record {

//===----------------------------------------------------------------------===//
// ACBS Specialization
//===----------------------------------------------------------------------===//
/// \memberof record::Subrecord<raw::ACBS, "ACBS"_rec>
template<> uint16_t ACBS::size() const {
  return 16u;
}

namespace raw {

template<> std::ostream &
write(std::ostream &os, const raw::ACBS &t, std::size_t /*size*/) {
  raw::write(os, t.flags, 0);
  io::writeBytes(os, t.baseSpellPoints);
  io::writeBytes(os, t.baseFatigue);
  io::writeBytes(os, t.barterGold);
  io::writeBytes(os, t.level);
  io::writeBytes(os, t.calcMin);
  io::writeBytes(os, t.calcMax);

  return os;
}

template<> std::istream &
read(std::istream &is, raw::ACBS &t, std::size_t /*size*/) {
  raw::read(is, t.flags, 0);
  io::readBytes(is, t.baseSpellPoints);
  io::readBytes(is, t.baseFatigue);
  io::readBytes(is, t.barterGold);
  io::readBytes(is, t.level);
  io::readBytes(is, t.calcMin);
  io::readBytes(is, t.calcMax);

  return is;
}

} // namespace raw

//===----------------------------------------------------------------------===//
// AIDT Specialization
//===----------------------------------------------------------------------===//
template<> uint16_t AIDT::size() const {
  return 12u;
}

namespace raw {

template<> std::ostream &
write(std::ostream &os, const raw::AIDT &t, std::size_t /*size*/) {
  io::writeBytes(os, t.aggression);
  io::writeBytes(os, t.confidence);
  io::writeBytes(os, t.energyLevel);
  io::writeBytes(os, t.responsibility);
  raw::write(os, t.flags, 0);
  io::writeBytes(os, t.trainingSkill);
  io::writeBytes(os, t.trainingLevel);
  io::writeBytes(os, t.unknown);

  return os;
}

template<> std::istream &
read(std::istream &is, raw::AIDT &t, std::size_t /*size*/) {
  io::readBytes(is, t.aggression);
  io::readBytes(is, t.confidence);
  io::readBytes(is, t.energyLevel);
  io::readBytes(is, t.responsibility);
  raw::read(is, t.flags, 0);
  io::readBytes(is, t.trainingSkill);
  io::readBytes(is, t.trainingLevel);
  io::readBytes(is, t.unknown);

  return is;
}

} // namespace raw

//===----------------------------------------------------------------------===//
// DATA_CLAS Specialization
//===----------------------------------------------------------------------===//
template<> uint16_t DATA_CLAS::size() const {
  return 4u * 12u + (data.hasTrainingInfo ? 4u : 0u);
}

namespace raw {

template<> std::ostream &
write(std::ostream &os, const raw::DATA_CLAS &t, std::size_t /*size*/) {
  io::writeBytes(os, t.primaryAttributes);
  io::writeBytes(os, t.specialization);
  io::writeBytes(os, t.majorSkills);
  raw::write(os, t.playableFlag, 0);
  raw::write(os, t.barterFlag, 0);
  if (t.hasTrainingInfo) {
    io::writeBytes(os, t.skillTrained);
    io::writeBytes(os, t.maxTrainingLevel);
    io::writeBytes(os, t.unused);
  }
  return os;
}

template<> std::istream &
read(std::istream &is, raw::DATA_CLAS &t, std::size_t size) {
  io::readBytes(is, t.primaryAttributes);
  io::readBytes(is, t.specialization);
  io::readBytes(is, t.majorSkills);
  raw::read(is, t.playableFlag, 0);
  raw::read(is, t.barterFlag, 0);
  if (size == 0x34) {
    t.hasTrainingInfo = true;
    io::readBytes(is, t.skillTrained);
    io::readBytes(is, t.maxTrainingLevel);
    io::readBytes(is, t.unused);
  }
  return is;
}

} // namespace raw

//===----------------------------------------------------------------------===//
// DATA_CONT Specialization
//===----------------------------------------------------------------------===//
template<> uint16_t DATA_CONT::size() const {
  return 5u;
}

namespace raw {

template<> std::ostream &
write(std::ostream &os, const raw::DATA_CONT &t, std::size_t /*size*/) {
  raw::write(os, t.flag, 0);
  io::writeBytes(os, t.weight);

  return os;
}

template<> std::istream &
read(std::istream &is, raw::DATA_CONT &t, std::size_t /*size*/) {
  raw::read(is, t.flag, 0);
  io::readBytes(is, t.weight);

  return is;
}

} // namespace raw

//===----------------------------------------------------------------------===//
// DATA_GMST Specialization
//===----------------------------------------------------------------------===//
template<> uint16_t DATA_GMST::size() const {
  return data.s.size() * 1u;
}

namespace raw {

template<> std::ostream &
write(std::ostream &os, const raw::DATA_GMST &t, std::size_t /*size*/) {
  os.write(t.s.data(), t.s.size() * 1);
  return os;
}

template<> std::istream &
read(std::istream &is, raw::DATA_GMST &t, std::size_t size) {
  t.s.clear();
  io::readBytes(is, t.s, size);
  if (size == 4) {
    t.i = *reinterpret_cast<int32_t *>(t.s.data());
    t.f = *reinterpret_cast<float *>(t.s.data());
  }
  return is;
}

} // namespace raw

//===----------------------------------------------------------------------===//
// DATA_GRAS Specialization
//===----------------------------------------------------------------------===//
template<> uint16_t DATA_GRAS::size() const {
  return 32u;
}

namespace raw {

template<> std::ostream &
write(std::ostream &os, const raw::DATA_GRAS &t, std::size_t /*size*/) {
  io::writeBytes(os, t.density);
  io::writeBytes(os, t.minSlope);
  io::writeBytes(os, t.maxSlope);
  io::writeBytes(os, t.unused1);
  io::writeBytes(os, t.unitsFromWater);
  io::writeBytes(os, t.unused2);
  io::writeBytes(os, t.positionRange);
  io::writeBytes(os, t.heightRange);
  io::writeBytes(os, t.colorRange);
  io::writeBytes(os, t.wavePeriod);
  io::writeBytes(os, t.flags);

  return os;
}

template<> std::istream &
read(std::istream &is, raw::DATA_GRAS &t, std::size_t size) {
  io::readBytes(is, t.density);
  io::readBytes(is, t.minSlope);
  io::readBytes(is, t.maxSlope);
  io::readBytes(is, t.unused1);
  io::readBytes(is, t.unitsFromWater);
  io::readBytes(is, t.unused2);
  io::readBytes(is, t.positionRange);
  io::readBytes(is, t.heightRange);
  io::readBytes(is, t.colorRange);
  io::readBytes(is, t.wavePeriod);
  io::readBytes(is, t.flags);

  return is;
}

} // namespace raw

//===----------------------------------------------------------------------===//
// DATA_LIGH Specialization
//===----------------------------------------------------------------------===//
template<> uint16_t DATA_LIGH::size() const {
  return 32u;
}

namespace raw {

template<> std::ostream &
write(std::ostream &os, const raw::DATA_LIGH &t, std::size_t /*size*/) {
  io::writeBytes(os, t.time);
  io::writeBytes(os, t.radius);
  io::writeBytes(os, t.color);
  raw::write(os, t.flags, 0);
  io::writeBytes(os, t.falloffExponent);
  io::writeBytes(os, t.fov);
  io::writeBytes(os, t.value);
  io::writeBytes(os, t.weight);

  return os;
}

template<> std::istream &
read(std::istream &is, raw::DATA_LIGH &t, std::size_t size) {
  io::readBytes(is, t.time);
  io::readBytes(is, t.radius);
  io::readBytes(is, t.color);
  raw::read(is, t.flags, 0);
  if (size == 32) {
    io::readBytes(is, t.falloffExponent);
    io::readBytes(is, t.fov);
  }
  io::readBytes(is, t.value);
  io::readBytes(is, t.weight);

  return is;
}

} // namespace raw

//===----------------------------------------------------------------------===//
// DATA_MGEF Specialization
//===----------------------------------------------------------------------===//
template<> uint16_t DATA_MGEF::size() const {
  return sizeof(raw::DATA_MGEF::Flag) + 4u + sizeof(oo::MagicSchool)
      + sizeof(oo::ActorValue) + 2u * sizeof(uint16_t) + 4u * sizeof(float)
      + 7u * sizeof(oo::FormId);
}

namespace raw {

template<> std::ostream &
write(std::ostream &os, const raw::DATA_MGEF &t, std::size_t /*size*/) {
  raw::write(os, t.flags, 0);
  io::writeBytes(os, t.baseCost);
  std::visit([&os](auto &&v) {
    io::writeBytes(os, v);
  }, t.associatedObject);
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
  return os;
}

template<> std::istream &
read(std::istream &is, raw::DATA_MGEF &t, std::size_t size) {
  raw::read(is, t.flags, 0);
  io::readBytes(is, t.baseCost);
  std::visit([&is](auto &&v) {
    io::readBytes(is, v);
  }, t.associatedObject);
  io::readBytes(is, t.school);
  io::readBytes(is, t.resist);
  io::readBytes(is, t.esceLength);
  io::readBytes(is, t.unused);
  io::readBytes(is, t.light);
  io::readBytes(is, t.projectileSpeed);
  io::readBytes(is, t.effectShader);
  // Blame the Darkness spell for this.
  if (size == 0x24) return is;
  io::readBytes(is, t.enchantEffect);
  io::readBytes(is, t.castingSound);
  io::readBytes(is, t.boltSound);
  io::readBytes(is, t.hitSound);
  io::readBytes(is, t.areaSound);
  io::readBytes(is, t.constantEffectEnchantmentFactor);
  io::readBytes(is, t.constantEffectBarterFactor);
  return is;
}

} // namespace raw

//===----------------------------------------------------------------------===//
// DATA_RACE Specialization
//===----------------------------------------------------------------------===//
template<> uint16_t DATA_RACE::size() const {
  return sizeof(oo::ActorValue) * 1u * data.skillModifiers.size() + 2u * 1u
      + 4u * 4u + 4u;
}

namespace raw {

template<> std::ostream &
write(std::ostream &os, const raw::DATA_RACE &t, std::size_t /*size*/) {
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
  raw::write(os, t.flags, 0);

  return os;
}

template<> std::istream &
read(std::istream &is, raw::DATA_RACE &t, std::size_t size) {
  // There can be zero to seven skill modifiers and they are not delimited, so
  // we have to compute how many there are using the total size.
  const auto allModifiersSize = size - 22u;
  const auto singleModifierSize = 2u;
  if (allModifiersSize % singleModifierSize != 0) {
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
  raw::read(is, t.flags, 0);

  return is;
}

} // namespace raw

//===----------------------------------------------------------------------===//
// DATA_WTHR Specialization
//===----------------------------------------------------------------------===//
template<> uint16_t DATA_WTHR::size() const {
  return 15u;
}

namespace raw {

template<> std::ostream &
write(std::ostream &os, const raw::DATA_WTHR &t, std::size_t /*size*/) {
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

  return os;
}

template<> std::istream &
read(std::istream &is, raw::DATA_WTHR &t, std::size_t /*size*/) {
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

  return is;
}

} // namespace raw

//===----------------------------------------------------------------------===//
// DELE Specialization
//===----------------------------------------------------------------------===//
template<> uint16_t DELE::size() const {
  return data.size;
}

namespace raw {

template<> std::ostream &
write(std::ostream &os, const raw::DELE &/*t*/, std::size_t size) {
  for (std::size_t i = 0; i < size; ++i) {
    os.put('\0');
  }
  return os;
}

template<> std::istream &
read(std::istream &is, raw::DELE &t, std::size_t size) {
  is.seekg(size, std::istream::cur);
  t.size = size;
  return is;
}

} // namespace raw

//===----------------------------------------------------------------------===//
// EFIT Specialization
//===----------------------------------------------------------------------===//
template<> uint16_t EFIT::size() const {
  return 5u * 4u + sizeof(oo::ActorValue);
}

namespace raw {

template<> std::ostream &
write(std::ostream &os, const raw::EFIT &t, std::size_t /*size*/) {
  io::writeBytes(os, t.efid);
  io::writeBytes(os, t.magnitude);
  io::writeBytes(os, t.area);
  io::writeBytes(os, t.duration);
  io::writeBytes(os, t.type);
  io::writeBytes(os, t.avIndex);

  return os;
}

template<> std::istream &
read(std::istream &is, raw::EFIT &t, std::size_t /*size*/) {
  io::readBytes(is, t.efid);
  io::readBytes(is, t.magnitude);
  io::readBytes(is, t.area);
  io::readBytes(is, t.duration);
  io::readBytes(is, t.type);
  io::readBytes(is, t.avIndex);

  return is;
}

} // namespace raw

//===----------------------------------------------------------------------===//
// ENAM Specialization
//===----------------------------------------------------------------------===//
template<> uint16_t ENAM::size() const {
  return data.eyes.size() * sizeof(oo::FormId);
}

namespace raw {

template<> std::ostream &
write(std::ostream &os, const raw::ENAM &t, std::size_t /*size*/) {
  for (const auto eye : t.eyes) {
    io::writeBytes(os, eye);
  }
  return os;
}

template<> std::istream &
read(std::istream &is, raw::ENAM &t, std::size_t size) {
  const std::size_t length = size / sizeof(oo::FormId);
  io::readBytes(is, t.eyes, length);
  return is;
}

} // namespace raw

//===----------------------------------------------------------------------===//
// ENIT Specialization
//===----------------------------------------------------------------------===//
template<> uint16_t ENIT::size() const {
  return 8;
}

namespace raw {

template<> std::ostream &
write(std::ostream &os, const raw::ENIT &t, std::size_t /*size*/) {
  io::writeBytes(os, t.value);
  raw::write(os, t.flags, 0);
  io::writeBytes(os, t.unused);

  return os;
}

template<> std::istream &
read(std::istream &is, raw::ENIT &t, std::size_t /*size*/) {
  io::readBytes(is, t.value);
  raw::read(is, t.flags, 0);
  io::readBytes(is, t.unused);
  return is;
}

} // namespace raw

//===----------------------------------------------------------------------===//
// ENIT_ENCH Specialization
//===----------------------------------------------------------------------===//
template<> uint16_t ENIT_ENCH::size() const {
  return 3u * 4u + 1u + 3u * 1u;
}

namespace raw {

template<> std::ostream &
write(std::ostream &os, const raw::ENIT_ENCH &t, std::size_t /*size*/) {
  io::writeBytes(os, t.type);
  io::writeBytes(os, t.chargeAmount);
  io::writeBytes(os, t.chargeCost);
  io::writeBytes(os, t.noAutoCalculate);
  io::writeBytes(os, t.unused);

  return os;
}

template<> std::istream &
read(std::istream &is, raw::ENIT_ENCH &t, std::size_t /*size*/) {
  io::readBytes(is, t.type);
  io::readBytes(is, t.chargeAmount);
  io::readBytes(is, t.chargeCost);
  io::readBytes(is, t.noAutoCalculate);
  io::readBytes(is, t.unused);

  return is;
}

} // namespace raw

//===----------------------------------------------------------------------===//
// ESCE Specialization
//===----------------------------------------------------------------------===//
template<> uint16_t ESCE::size() const {
  return data.effects.size() * sizeof(oo::EffectId);
}

namespace raw {

template<> std::ostream &
write(std::ostream &os, const raw::ESCE &t, std::size_t /*size*/) {
  for (const auto &effect : t.effects) {
    io::writeBytes(os, effect);
  }
  return os;
}

template<> std::istream &
read(std::istream &is, raw::ESCE &t, std::size_t size) {
  const std::size_t length = size / sizeof(oo::EffectId);
  io::readBytes(is, t.effects, length);
  return is;
}

} // namespace raw

//===----------------------------------------------------------------------===//
// HNAM Specialization
//===----------------------------------------------------------------------===//
template<> uint16_t HNAM::size() const {
  return data.hair.size() * sizeof(oo::FormId);
}

namespace raw {

template<> std::ostream &
write(std::ostream &os, const raw::HNAM &t, std::size_t /*size*/) {
  for (const auto hair : t.hair) {
    io::writeBytes(os, hair);
  }
  return os;
}

template<> std::istream &
read(std::istream &is, raw::HNAM &t, std::size_t size) {
  const std::size_t length = size / sizeof(oo::FormId);
  io::readBytes(is, t.hair, length);
  return is;
}

} // namespace raw

//===----------------------------------------------------------------------===//
// HNAM_LTEX Specialization
//===----------------------------------------------------------------------===//
template<> uint16_t HNAM_LTEX::size() const {
  return 3u * 1u;
}

namespace raw {

template<> std::ostream &
write(std::ostream &os, const raw::HNAM_LTEX &t, std::size_t /*size*/) {
  io::writeBytes(os, t.type);
  io::writeBytes(os, t.friction);
  io::writeBytes(os, t.restitution);
  return os;
}

template<> std::istream &
read(std::istream &is, raw::HNAM_LTEX &t, std::size_t /*size*/) {
  io::readBytes(is, t.type);
  io::readBytes(is, t.friction);
  io::readBytes(is, t.restitution);
  return is;
}

} // namespace raw

//===----------------------------------------------------------------------===//
// MNAM_WRLD Specialization
//===----------------------------------------------------------------------===//
template<> uint16_t MNAM_WRLD::size() const {
  return 16u;
}

namespace raw {

template<> std::ostream &
write(std::ostream &os, const raw::MNAM_WRLD &t, std::size_t /*size*/) {
  io::writeBytes(os, t.width);
  io::writeBytes(os, t.height);
  io::writeBytes(os, t.topLeft.x);
  io::writeBytes(os, t.topLeft.y);
  io::writeBytes(os, t.bottomRight.x);
  io::writeBytes(os, t.bottomRight.y);

  return os;
}

template<> std::istream &
read(std::istream &is, raw::MNAM_WRLD &t, std::size_t size) {
  io::readBytes(is, t.width);
  io::readBytes(is, t.height);
  io::readBytes(is, t.topLeft.x);
  io::readBytes(is, t.topLeft.y);
  io::readBytes(is, t.bottomRight.x);
  io::readBytes(is, t.bottomRight.y);

  return is;
}

} // namespace raw

//===----------------------------------------------------------------------===//
// MODT Specialization
//===----------------------------------------------------------------------===//
template<> uint16_t MODT::size() const {
  return 3u * 8u * data.records.size();
}

namespace raw {

template<> std::ostream &
write(std::ostream &os, const raw::MODT &t, std::size_t /*size*/) {
  for (const auto &record : t.records) {
    io::writeBytes(os, record.ddsHash);
    io::writeBytes(os, record.ddxHash);
    io::writeBytes(os, record.folderHash);
  }
  return os;
}

template<> std::istream &
read(std::istream &is, raw::MODT &t, std::size_t size) {
  t.records.reserve(size / (3 * 8));
  for (std::size_t i = 0; i < size / (3 * 8); ++i) {
    raw::MODT::MODTRecord record;
    io::readBytes(is, record.ddsHash);
    io::readBytes(is, record.ddxHash);
    io::readBytes(is, record.folderHash);
    t.records.emplace_back(record);
  }
  return is;
}

} // namespace raw

//===----------------------------------------------------------------------===//
// NAM0_WTHR Specialization
//===----------------------------------------------------------------------===//
template<> uint16_t NAM0_WTHR::size() const {
  return 10u * 4u * 4u;
}

namespace raw {

template<> std::ostream &
write(std::ostream &os, const raw::NAM0_WTHR &t, std::size_t /*size*/) {
  auto writeColors = [&os](const raw::NAM0_WTHR::WeatherColors &c) {
    io::writeBytes(os, c.sunrise);
    io::writeBytes(os, c.day);
    io::writeBytes(os, c.sunset);
    io::writeBytes(os, c.night);
  };

  writeColors(t.skyUpper);
  writeColors(t.fog);
  writeColors(t.cloudsLower);
  writeColors(t.ambient);
  writeColors(t.sunlight);
  writeColors(t.sun);
  writeColors(t.stars);
  writeColors(t.skyLower);
  writeColors(t.horizon);
  writeColors(t.cloudsUpper);

  return os;
}

template<> std::istream &
read(std::istream &is, raw::NAM0_WTHR &t, std::size_t /*size*/) {
  auto readColors = [&is](raw::NAM0_WTHR::WeatherColors &c) {
    io::readBytes(is, c.sunrise);
    io::readBytes(is, c.day);
    io::readBytes(is, c.sunset);
    io::readBytes(is, c.night);
  };

  readColors(t.skyUpper);
  readColors(t.fog);
  readColors(t.cloudsLower);
  readColors(t.ambient);
  readColors(t.sunlight);
  readColors(t.sun);
  readColors(t.stars);
  readColors(t.skyLower);
  readColors(t.horizon);
  readColors(t.cloudsUpper);

  return is;
}

} // namespace raw

//===----------------------------------------------------------------------===//
// OFST Specialization
//===----------------------------------------------------------------------===//
template<> uint16_t OFST::size() const {
  return 3u * 4u * data.unused.size();
}

namespace raw {

template<> std::ostream &
write(std::ostream &os, const raw::OFST &t, std::size_t /*size*/) {
  for (const auto &entry : t.unused) {
    io::writeBytes(os, entry);
  }
  return os;
}

template<> std::istream &
read(std::istream &is, raw::OFST &t, std::size_t size) {
  for (std::size_t i = 0; i < size; i += 3 * 4) {
    std::array<uint32_t, 3> entry = {};
    is.read(reinterpret_cast<char *>(entry.data()), 3 * 4);
    t.unused.push_back(entry);
  }
  return is;
}

} // namespace raw

//===----------------------------------------------------------------------===//
// OFST_WRLD Specialization
//===----------------------------------------------------------------------===//
template<> uint16_t OFST_WRLD::size() const {
  return data.entries.size() * 4u;
}

namespace raw {

template<> std::ostream &
write(std::ostream &os, const raw::OFST_WRLD &t, std::size_t /*size*/) {
  for (const auto entry : t.entries) io::writeBytes(os, entry);
  return os;
}

template<> std::istream &
read(std::istream &is, raw::OFST_WRLD &t, std::size_t size) {
  const std::size_t length = size / 4u;
  io::readBytes(is, t.entries, length);
  return is;
}

} // namespace raw

//===----------------------------------------------------------------------===//
// PFPC Specialization
//===----------------------------------------------------------------------===//
template<> uint16_t PFPC::size() const {
  return 4u;
}

namespace raw {

template<> std::ostream &
write(std::ostream &os, const raw::PFPC &t, std::size_t /*size*/) {
  io::writeBytes(os, t.springChance);
  io::writeBytes(os, t.summerChance);
  io::writeBytes(os, t.autumnChance);
  io::writeBytes(os, t.winterChance);

  return os;
}

template<> std::istream &
read(std::istream &is, raw::PFPC &t, std::size_t /*size*/) {
  io::readBytes(is, t.springChance);
  io::readBytes(is, t.summerChance);
  io::readBytes(is, t.autumnChance);
  io::readBytes(is, t.winterChance);

  return is;
}

} // namespace raw

//===----------------------------------------------------------------------===//
// SCIT Specialization
//===----------------------------------------------------------------------===//
template<> uint16_t SCIT::size() const {
  return sizeof(oo::FormId) + sizeof(oo::MagicSchool) + 8u;
}

namespace raw {

template<> std::ostream &
write(std::ostream &os, const raw::SCIT &t, std::size_t /*size*/) {
  io::writeBytes(os, t.id);
  io::writeBytes(os, t.school);
  io::writeBytes(os, t.visualEffect);
  raw::write(os, t.flags, 0);
  io::writeBytes(os, t.unused);

  return os;
}

template<> std::istream &
read(std::istream &is, raw::SCIT &t, std::size_t /*size*/) {
  io::readBytes(is, t.id);
  io::readBytes(is, t.school);
  io::readBytes(is, t.visualEffect);
  raw::read(is, t.flags, 0);
  io::readBytes(is, t.unused);

  return is;
}

} // namespace raw

//===----------------------------------------------------------------------===//
// SNAM_TREE Specialization
//===----------------------------------------------------------------------===//
template<> uint16_t SNAM_TREE::size() const {
  return 4u * data.seeds.size();
}

namespace raw {

template<> std::ostream &
write(std::ostream &os, const raw::SNAM_TREE &t, std::size_t /*size*/) {
  for (auto s : t.seeds) io::writeBytes(os, s);
  return os;
}

template<> std::istream &
read(std::istream &is, raw::SNAM_TREE &t, std::size_t size) {
  io::readBytes(is, t.seeds, size / 4u);
  return is;
}

} // namespace raw

//===----------------------------------------------------------------------===//
// SNAM_WTHR Specialization
//===----------------------------------------------------------------------===//
template<> uint16_t SNAM_WTHR::size() const {
  return 8u;
}

namespace raw {

template<> std::ostream &
write(std::ostream &os, const raw::SNAM_WTHR &t, std::size_t /*size*/) {
  io::writeBytes(os, t.soundId);
  io::writeBytes(os, t.soundType);
  return os;
}

template<> std::istream &
read(std::istream &is, raw::SNAM_WTHR &t, std::size_t /*size*/) {
  io::readBytes(is, t.soundId);
  io::readBytes(is, t.soundType);
  return is;
}

} // namespace raw

//===----------------------------------------------------------------------===//
// SNDD Specialization
//===----------------------------------------------------------------------===//
template<> uint16_t SNDD::size() const {
  return 4u * 1u + 4u;
};

namespace raw {

template<> std::ostream &
write(std::ostream &os, const raw::SNDD &t, std::size_t /*size*/) {
  io::writeBytes(os, t.minAttenuationDistance);
  io::writeBytes(os, t.maxAttenuationDistance);
  io::writeBytes(os, t.frequencyAdjustment);
  io::writeBytes(os, t.unused);
  raw::write(os, t.flags, 0);
  return os;
}

template<> std::istream &
read(std::istream &is, raw::SNDD &t, std::size_t /*size*/) {
  io::readBytes(is, t.minAttenuationDistance);
  io::readBytes(is, t.maxAttenuationDistance);
  io::readBytes(is, t.frequencyAdjustment);
  io::readBytes(is, t.unused);
  raw::read(is, t.flags, 0);
  return is;
}

} // namespace raw

//===----------------------------------------------------------------------===//
// SNDX Specialization
//===----------------------------------------------------------------------===//
template<> uint16_t SNDX::size() const {
  return 4u * 1u + 2u * 4u + (data.staticAttenuation ? 4u : 0u)
      + (data.startTime ? 4u : 0u) + (data.stopTime ? 4u : 0u);
}

namespace raw {

template<> std::ostream &
write(std::ostream &os, const raw::SNDX &t, std::size_t /*size*/) {
  io::writeBytes(os, t.minAttenuationDistance);
  io::writeBytes(os, t.maxAttenuationDistance);
  io::writeBytes(os, t.frequencyAdjustment);
  io::writeBytes(os, t.unused);
  raw::write(os, t.flags, 0);
  io::writeBytes(os, t.unusedWord);
  io::writeBytes(os, t.staticAttenuation);
  io::writeBytes(os, t.startTime);
  io::writeBytes(os, t.stopTime);
  return os;
}

template<> std::istream &
read(std::istream &is, raw::SNDX &t, std::size_t size) {
  io::readBytes(is, t.minAttenuationDistance);
  io::readBytes(is, t.maxAttenuationDistance);
  io::readBytes(is, t.frequencyAdjustment);
  io::readBytes(is, t.unused);
  read(is, t.flags, 0);
  io::readBytes(is, t.unusedWord);
  if (size > 12) io::readBytes(is, t.staticAttenuation);
  if (size > 16) io::readBytes(is, t.startTime);
  if (size > 17) io::readBytes(is, t.stopTime);
  return is;
}

} // namespace raw

//===----------------------------------------------------------------------===//
// SPIT Specialization
//===----------------------------------------------------------------------===//
template<> uint16_t SPIT::size() const {
  return 16u;
}

namespace raw {

template<> std::ostream &
write(std::ostream &os, const raw::SPIT &t, std::size_t /*size*/) {
  io::writeBytes(os, t.type);
  io::writeBytes(os, t.cost);
  io::writeBytes(os, t.level);
  raw::write(os, t.flags, 0);

  return os;
}

template<> std::istream &
read(std::istream &is, raw::SPIT &t, std::size_t /*size*/) {
  io::readBytes(is, t.type);
  io::readBytes(is, t.cost);
  io::readBytes(is, t.level);
  raw::read(is, t.flags, 0);

  return is;
}

} // namespace raw

//===----------------------------------------------------------------------===//
// TNAM_CLMT Specialization
//===----------------------------------------------------------------------===//
template<> uint16_t TNAM_CLMT::size() const {
  return 6u;
}

namespace raw {

template<> std::ostream &
write(std::ostream &os, const raw::TNAM_CLMT &t, std::size_t /*size*/) {
  io::writeBytes(os, t.sunriseBegin);
  io::writeBytes(os, t.sunriseEnd);
  io::writeBytes(os, t.sunsetBegin);
  io::writeBytes(os, t.sunsetEnd);
  io::writeBytes(os, t.volatility);

  auto flag{static_cast<uint8_t>(
                (t.hasMasser << 7u) & (t.hasSecunda << 6u) & t.phaseLength)};
  io::writeBytes(os, flag);

  return os;
}

template<> std::istream &
read(std::istream &is, raw::TNAM_CLMT &t, std::size_t /*size*/) {
  io::readBytes(is, t.sunriseBegin);
  io::readBytes(is, t.sunriseEnd);
  io::readBytes(is, t.sunsetBegin);
  io::readBytes(is, t.sunsetEnd);
  io::readBytes(is, t.volatility);

  uint8_t flag;
  io::readBytes(is, flag);
  t.hasMasser = flag >> 7u;
  t.hasSecunda = (flag & 0b01000000) >> 6u;
  t.phaseLength = flag & 0b00111111;

  return is;
}

} // namespace raw

//===----------------------------------------------------------------------===//
// VTXT Specialization
//===----------------------------------------------------------------------===//
template<> uint16_t VTXT::size() const {
  return data.points.size() * 8u;
}

namespace raw {

template<> std::ostream &
write(std::ostream &os, const raw::VTXT &t, std::size_t /*size*/) {
  for (const auto &p : t.points) {
    io::writeBytes(os, p.position);
    io::writeBytes(os, p.unused);
    io::writeBytes(os, p.opacity);
  }

  return os;
}

template<> std::istream &
read(std::istream &is, raw::VTXT &t, std::size_t size) {
  t.points.resize(size / 8u);
  is.read(reinterpret_cast<char *>(t.points.data()), size);

  return is;
}

} // namespace raw

//===----------------------------------------------------------------------===//
// WLST Specialization
//===----------------------------------------------------------------------===//
template<> uint16_t WLST::size() const {
  return data.weathers.size() * 8u;
}

namespace raw {

template<> std::ostream &
write(std::ostream &os, const raw::WLST &t, std::size_t /*size*/) {
  for (auto weather : t.weathers) {
    io::writeBytes(os, weather.formId);
    io::writeBytes(os, weather.chance);
  }

  return os;
}

template<> std::istream &
read(std::istream &is, raw::WLST &t, std::size_t size) {
  t.weathers.resize(size / 8u);
  for (std::size_t i = 0; i < size / 8u; ++i) {
    io::readBytes(is, t.weathers[i].formId);
    io::readBytes(is, t.weathers[i].chance);
  }

  return is;
}

} // namespace raw

//===----------------------------------------------------------------------===//
// XCLR Specialization
//===----------------------------------------------------------------------===//
template<> uint16_t XCLR::size() const {
  return sizeof(oo::FormId) * data.regions.size();
}

namespace raw {

template<> std::ostream &
write(std::ostream &os, const raw::XCLR &t, std::size_t /*size*/) {
  for (const auto &region : t.regions) {
    io::writeBytes(os, region);
  }
  return os;
}

template<> std::istream &
read(std::istream &is, raw::XCLR &t, std::size_t size) {
  const std::size_t length = size / sizeof(oo::FormId);
  io::readBytes(is, t.regions, length);
  return is;
}

} // namespace raw

//===----------------------------------------------------------------------===//
// XESP Specialization
//===----------------------------------------------------------------------===//
template<> uint16_t XESP::size() const {
  return sizeof(oo::FormId) + sizeof(raw::XESP::Flag);
}

namespace raw {

template<> std::ostream &
write(std::ostream &os, const raw::XESP &t, std::size_t /*size*/) {
  io::writeBytes(os, t.parent);
  raw::write(os, t.flags, 0);
  return os;
}

template<> std::istream &
read(std::istream &is, raw::XESP &t, std::size_t /*size*/) {
  io::readBytes(is, t.parent);
  raw::read(is, t.flags, 0);
  return is;
}

} // namespace raw

//===-----------------------------------------------------------------------==//
// XLOC Specialization
//===-----------------------------------------------------------------------==//
template<> uint16_t XLOC::size() const {
  return 4u + sizeof(oo::FormId) + 4u;
}

namespace raw {

template<> std::ostream &
write(std::ostream &os, const raw::XLOC &t, std::size_t /*size*/) {
  io::writeBytes(os, t.lockLevel);
  io::writeBytes(os, t.key);
  //io::writeBytes(os, t.unused);
  raw::write(os, t.flags, 0);
  return os;
}

template<> std::istream &
read(std::istream &is, raw::XLOC &t, std::size_t size) {
  io::readBytes(is, t.lockLevel);
  io::readBytes(is, t.key);
  if (size == 16) io::readBytes(is, t.unused);
  raw::read(is, t.flags, 0);
  return is;
}

} // namespace raw

//===----------------------------------------------------------------------===//
// XRGD Specialization
//===----------------------------------------------------------------------===//
template<> uint16_t XRGD::size() const {
  return data.bytes.size() * 1u;
}

namespace raw {

template<> std::ostream &
write(std::ostream &os, const raw::XRGD &t, std::size_t /*size*/) {
  for (const auto byte : t.bytes) {
    io::writeBytes(os, byte);
  }
  return os;
}

template<> std::istream &
read(std::istream &is, raw::XRGD &t, std::size_t size) {
  const std::size_t length = size / 1u;
  io::readBytes(is, t.bytes, length);
  return is;
}

} // namespace raw

//===----------------------------------------------------------------------===//
// XSED Specialization
//===----------------------------------------------------------------------===//
template<> uint16_t XSED::size() const {
  return data.size;
}

namespace raw {

template<> std::ostream &
write(std::ostream &os, const raw::XSED &t, std::size_t size) {
  for (auto i = 0; i < size; ++i) {
    os.put(0);
  }
  return os;
}

template<> std::istream &
read(std::istream &is, raw::XSED &t, std::size_t size) {
  t.size = size;
  is.seekg(size, std::ios_base::cur);
  return is;
}

} // namespace raw

} // namespace record;
