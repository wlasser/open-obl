#include "io/write_bytes.hpp"
#include "io/read_bytes.hpp"
#include "record/record.hpp"
#include "subrecords.hpp"
#include <istream>
#include <ostream>

namespace record {

using namespace io;

// SPIT specialization
template<>
uint16_t SPIT::size() const {
  return 16u;
}

template<>
std::ostream &raw::write(std::ostream &os,
                         const raw::SPIT &t,
                         std::size_t /*size*/) {
  writeBytes(os, t.type);
  writeBytes(os, t.cost);
  writeBytes(os, t.level);
  writeBytes(os, t.flags);

  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::SPIT &t, std::size_t /*size*/) {
  readBytes(is, t.type);
  readBytes(is, t.cost);
  readBytes(is, t.level);
  readBytes(is, t.flags);

  return is;
}

// XSED specialization
template<>
uint16_t XSED::size() const {
  return data.size;
}

template<>
std::ostream &raw::write(std::ostream &os,
                         const raw::XSED &t,
                         std::size_t size) {
  for (auto i = 0; i < size; ++i) {
    os.put(0);
  }
  return os;
}

template<>
std::istream &raw::read(std::istream &is,
                        raw::XSED &t,
                        std::size_t size) {
  t.size = size;
  is.seekg(size, std::ios_base::cur);
  return is;
}

// XRGD specialization
template<>
uint16_t XRGD::size() const {
  return data.bytes.size() * 1u;
}

template<>
std::ostream &raw::write(std::ostream &os,
                         const raw::XRGD &t,
                         std::size_t /*size*/) {
  for (const auto byte : t.bytes) {
    writeBytes(os, byte);
  }
  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::XRGD &t, std::size_t size) {
  const std::size_t length = size / 1u;
  readBytes(is, t.bytes, length);
  return is;
}

// XLOC specicalization
template<>
uint16_t XLOC::size() const {
  return 4u + sizeof(FormId) + 4u;
}

template<>
std::ostream &raw::write(std::ostream &os,
                         const raw::XLOC &t,
                         std::size_t /*size*/) {
  writeBytes(os, t.lockLevel);
  writeBytes(os, t.key);
  //writeBytes(os, t.unused);
  writeBytes(os, t.flags);
  return os;
}

template<>
std::istream &raw::read(std::istream &is,
                        raw::XLOC &t,
                        std::size_t size) {
  readBytes(is, t.lockLevel);
  readBytes(is, t.key);
  if (size == 16) readBytes(is, t.unused);
  readBytes(is, t.flags);
  return is;
}

// XESP specialization
template<>
uint16_t XESP::size() const {
  return sizeof(FormId) + sizeof(raw::XESP::Flag);
}

template<>
std::ostream &raw::write(std::ostream &os,
                         const raw::XESP &t,
                         std::size_t /*size*/) {
  writeBytes(os, t.parent);
  writeBytes(os, t.flags);
  return os;
}

template<>
std::istream &raw::read(std::istream &is,
                        raw::XESP &t,
                        std::size_t /*size*/) {
  readBytes(is, t.parent);
  readBytes(is, t.flags);
  return is;
}

// XCLR specialization
template<>
uint16_t XCLR::size() const {
  return sizeof(FormId) * data.regions.size();
}

template<>
std::ostream &raw::write(std::ostream &os,
                         const raw::XCLR &t,
                         std::size_t /*size*/) {
  for (const auto &region : t.regions) {
    writeBytes(os, region);
  }
  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::XCLR &t, std::size_t size) {
  const std::size_t length = size / sizeof(FormId);
  io::readBytes(is, t.regions, length);
  return is;
}

// HNAM_LTEX specialization
template<>
uint16_t HNAM_LTEX::size() const {
  return 3u * 1u;
}

template<>
std::ostream &raw::write(std::ostream &os,
                         const raw::HNAM_LTEX &t,
                         std::size_t /*size*/) {
  writeBytes(os, t.type);
  writeBytes(os, t.friction);
  writeBytes(os, t.restitution);
  return os;
}

template<>
std::istream &raw::read(std::istream &is,
                        raw::HNAM_LTEX &t,
                        std::size_t /*size*/) {
  readBytes(is, t.type);
  readBytes(is, t.friction);
  readBytes(is, t.restitution);
  return is;
}

// HNAM specialization
template<>
uint16_t HNAM::size() const {
  return data.hair.size() * sizeof(FormId);
}

template<>
std::ostream &raw::write(std::ostream &os,
                         const raw::HNAM &t,
                         std::size_t /*size*/) {
  for (const auto hair : t.hair) {
    writeBytes(os, hair);
  }
  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::HNAM &t, std::size_t size) {
  const std::size_t length = size / sizeof(FormId);
  io::readBytes(is, t.hair, length);
  return is;
}

// ENAM specialization
template<>
uint16_t ENAM::size() const {
  return data.eyes.size() * sizeof(FormId);
}

template<>
std::ostream &raw::write(std::ostream &os,
                         const raw::ENAM &t,
                         std::size_t /*size*/) {
  for (const auto eye : t.eyes) {
    writeBytes(os, eye);
  }
  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::ENAM &t, std::size_t size) {
  const std::size_t length = size / sizeof(FormId);
  io::readBytes(is, t.eyes, length);
  return is;
}

// ESCE specialization
template<>
uint16_t ESCE::size() const {
  return data.effects.size() * sizeof(EffectID);
}

template<>
std::ostream &raw::write(std::ostream &os,
                         const raw::ESCE &t,
                         std::size_t /*size*/) {
  for (const auto &effect : t.effects) {
    writeBytes(os, effect);
  }
  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::ESCE &t, std::size_t size) {
  const std::size_t length = size / sizeof(EffectID);
  io::readBytes(is, t.effects, length);
  return is;
}

// SNDD specialization
template<>
uint16_t SNDD::size() const {
  return 4u * 1u + 4u;
};

template<>
std::ostream &raw::write(std::ostream &os,
                         const raw::SNDD &t,
                         std::size_t /*size*/) {
  writeBytes(os, t.minAttenuationDistance);
  writeBytes(os, t.maxAttenuationDistance);
  writeBytes(os, t.frequencyAdjustment);
  writeBytes(os, t.unused);
  writeBytes(os, t.flags);
  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::SNDD &t, std::size_t /*size*/) {
  readBytes(is, t.minAttenuationDistance);
  readBytes(is, t.maxAttenuationDistance);
  readBytes(is, t.frequencyAdjustment);
  readBytes(is, t.unused);
  readBytes(is, t.flags);
  return is;
}

// SNDX specialization
template<>
uint16_t SNDX::size() const {
  return 4u * 1u + 2u * 4u + (data.staticAttenuation ? 4u : 0u)
      + (data.startTime ? 4u : 0u) + (data.stopTime ? 4u : 0u);
}

template<>
std::ostream &raw::write(std::ostream &os,
                         const raw::SNDX &t,
                         std::size_t /*size*/) {
  writeBytes(os, t.minAttenuationDistance);
  writeBytes(os, t.maxAttenuationDistance);
  writeBytes(os, t.frequencyAdjustment);
  writeBytes(os, t.unused);
  writeBytes(os, t.flags);
  writeBytes(os, t.unusedWord);
  writeBytes(os, t.staticAttenuation);
  writeBytes(os, t.startTime);
  writeBytes(os, t.stopTime);
  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::SNDX &t, std::size_t size) {
  readBytes(is, t.minAttenuationDistance);
  readBytes(is, t.maxAttenuationDistance);
  readBytes(is, t.frequencyAdjustment);
  readBytes(is, t.unused);
  readBytes(is, t.flags);
  readBytes(is, t.unusedWord);
  if (size > 12) readBytes(is, t.staticAttenuation);
  if (size > 16) readBytes(is, t.startTime);
  if (size > 17) readBytes(is, t.stopTime);
  return is;
}

// DATA_MGEF specialization
template<>
uint16_t DATA_MGEF::size() const {
  return sizeof(raw::DATA_MGEF::Flag) + sizeof(raw::DATA_MGEF::AssociatedObject)
      + sizeof(MagicSchool) + sizeof(ActorValue) + 2u * sizeof(uint16_t)
      + 4u * sizeof(float) + 7u * sizeof(FormId);
}

template<>
std::ostream &raw::write(std::ostream &os,
                         const raw::DATA_MGEF &t,
                         std::size_t /*size*/) {
  writeBytes(os, t.flags);
  writeBytes(os, t.baseCost);
  writeBytes(os, t.associatedObject);
  writeBytes(os, t.school);
  writeBytes(os, t.resist);
  writeBytes(os, t.esceLength);
  writeBytes(os, t.unused);
  writeBytes(os, t.light);
  writeBytes(os, t.projectileSpeed);
  writeBytes(os, t.effectShader);
  writeBytes(os, t.enchantEffect);
  writeBytes(os, t.castingSound);
  writeBytes(os, t.boltSound);
  writeBytes(os, t.hitSound);
  writeBytes(os, t.areaSound);
  writeBytes(os, t.constantEffectEnchantmentFactor);
  writeBytes(os, t.constantEffectBarterFactor);
  return os;
}

template<>
std::istream &raw::read(std::istream &is,
                        raw::DATA_MGEF &t,
                        std::size_t size) {
  readBytes(is, t.flags);
  readBytes(is, t.baseCost);
  readBytes(is, t.associatedObject);
  readBytes(is, t.school);
  readBytes(is, t.resist);
  readBytes(is, t.esceLength);
  readBytes(is, t.unused);
  readBytes(is, t.light);
  readBytes(is, t.projectileSpeed);
  readBytes(is, t.effectShader);
  // Blame the Darkness spell for this.
  if (size == 0x24) return is;
  readBytes(is, t.enchantEffect);
  readBytes(is, t.castingSound);
  readBytes(is, t.boltSound);
  readBytes(is, t.hitSound);
  readBytes(is, t.areaSound);
  readBytes(is, t.constantEffectEnchantmentFactor);
  readBytes(is, t.constantEffectBarterFactor);
  return is;
}

// DATA_RACE specialization
template<>
uint16_t DATA_RACE::size() const {
  return sizeof(ActorValue) * 1u * data.skillModifiers.size() + 2u * 1u
      + 4u * 4u + 4u;
}

template<>
std::ostream &raw::write(std::ostream &os,
                         const raw::DATA_RACE &t,
                         std::size_t /*size*/) {
  for (const auto &pair : t.skillModifiers) {
    // Despite being an ActorValue, only the first byte is used
    auto av = static_cast<uint8_t>(pair.first);
    writeBytes(os, av);
    writeBytes(os, pair.second);
  }
  writeBytes(os, t.unused);
  writeBytes(os, t.heightMale);
  writeBytes(os, t.heightFemale);
  writeBytes(os, t.heightFemale);
  writeBytes(os, t.weightMale);
  writeBytes(os, t.weightFemale);
  writeBytes(os, t.flags);

  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::DATA_RACE &t, std::size_t size) {
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
    readBytes(is, p);
    t.skillModifiers.emplace_back(static_cast<ActorValue>(p.first), p.second);
  }
  readBytes(is, t.unused);
  readBytes(is, t.heightMale);
  readBytes(is, t.heightFemale);
  readBytes(is, t.weightMale);
  readBytes(is, t.weightFemale);
  readBytes(is, t.flags);

  return is;
}

// DATA_CLAS specialization
template<>
uint16_t DATA_CLAS::size() const {
  return 4u * 12u + (data.hasTrainingInfo ? 4u : 0u);
}

template<>
std::ostream &raw::write(std::ostream &os, const raw::DATA_CLAS &t,
                         std::size_t /*size*/) {
  writeBytes(os, t.primaryAttributes);
  writeBytes(os, t.specialization);
  writeBytes(os, t.majorSkills);
  writeBytes(os, t.playableFlag);
  writeBytes(os, t.barterFlag);
  if (t.hasTrainingInfo) {
    writeBytes(os, t.skillTrained);
    writeBytes(os, t.maxTrainingLevel);
    writeBytes(os, t.unused);
  }
  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::DATA_CLAS &t, std::size_t size) {
  readBytes(is, t.primaryAttributes);
  readBytes(is, t.specialization);
  readBytes(is, t.majorSkills);
  readBytes(is, t.playableFlag);
  readBytes(is, t.barterFlag);
  if (size == 0x34) {
    t.hasTrainingInfo = true;
    readBytes(is, t.skillTrained);
    readBytes(is, t.maxTrainingLevel);
    readBytes(is, t.unused);
  }
  return is;
}

// DATA_GMST specialization
template<>
uint16_t DATA_GMST::size() const {
  return data.s.size() * 1u;
}

template<>
std::ostream &raw::write(std::ostream &os, const raw::DATA_GMST &t,
                         std::size_t /*size*/) {
  os.write(t.s.data(), t.s.size() * 1);
  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::DATA_GMST &t, std::size_t size) {
  t.s = std::vector<char>(size, '\0');
  readOrThrow(is, t.s.data(), size * sizeof(char), "DATA_GMST");
  if (size == 4) {
    t.i = *reinterpret_cast<int32_t *>(t.s.data());
    t.f = *reinterpret_cast<float *>(t.s.data());
  }
  return is;
}

// DATA_LIGH specialization
template<>
uint16_t DATA_LIGH::size() const {
  return 32u;
}

template<>
std::ostream &raw::write(std::ostream &os, const raw::DATA_LIGH &t,
                         std::size_t /*size*/) {
  writeBytes(os, t.time);
  writeBytes(os, t.radius);
  writeBytes(os, t.color);
  writeBytes(os, t.flags);
  writeBytes(os, t.falloffExponent);
  writeBytes(os, t.fov);
  writeBytes(os, t.value);
  writeBytes(os, t.weight);

  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::DATA_LIGH &t, std::size_t size) {
  readBytes(is, t.time);
  readBytes(is, t.radius);
  readBytes(is, t.color);
  readBytes(is, t.flags);
  if (size == 32) {
    readBytes(is, t.falloffExponent);
    readBytes(is, t.fov);
  }
  readBytes(is, t.value);
  readBytes(is, t.weight);

  return is;
}

// EFID specialization
template<>
uint16_t EFID::size() const {
  return 4;
}

template<>
std::ostream &raw::write(std::ostream &os,
                         const raw::EFID &t,
                         std::size_t /*size*/) {
  writeBytes(os, t);
  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::EFID &t, std::size_t /*size*/) {
  readBytes(is, t);
  return is;
}

// OFST specialization
template<>
uint16_t OFST::size() const {
  return 3u * 4u * data.unused.size();
}

template<>
std::ostream &raw::write(std::ostream &os,
                         const raw::OFST &t,
                         std::size_t /*size*/) {
  for (const auto &entry : t.unused) {
    writeBytes(os, entry);
  }
  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::OFST &t, std::size_t size) {
  for (std::size_t i = 0; i < size; i += 3 * 4) {
    std::array<uint32_t, 3> entry = {};
    is.read(reinterpret_cast<char *>(entry.data()), 3 * 4);
    t.unused.push_back(entry);
  }
  return is;
}

// DELE specialization
template<>
uint16_t DELE::size() const {
  return data.size;
}

template<>
std::ostream &raw::write(std::ostream &os,
                         const raw::DELE &/*t*/,
                         std::size_t size) {
  for (std::size_t i = 0; i < size; ++i) {
    os.put('\0');
  }
  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::DELE &t, std::size_t size) {
  is.seekg(size, std::istream::cur);
  t.size = size;
  return is;
}

// MODT specialization
template<>
uint16_t MODT::size() const {
  return 3u * 8u * data.records.size();
}

template<>
std::ostream &raw::write(std::ostream &os,
                         const raw::MODT &t,
                         std::size_t /*size*/) {
  for (const auto &record : t.records) {
    writeBytes(os, record.ddsHash);
    writeBytes(os, record.ddxHash);
    writeBytes(os, record.folderHash);
  }
  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::MODT &t, std::size_t size) {
  t.records.reserve(size / (3 * 8));
  for (std::size_t i = 0; i < size / (3 * 8); ++i) {
    raw::MODT::MODTRecord record;
    readBytes(is, record.ddsHash);
    readBytes(is, record.ddxHash);
    readBytes(is, record.folderHash);
    t.records.emplace_back(record);
  }
  return is;
}

// ENIT specialization
template<>
uint16_t ENIT::size() const {
  return 8;
}

template<>
std::ostream &raw::write(std::ostream &os,
                         const raw::ENIT &t,
                         std::size_t /*size*/) {
  writeBytes(os, t.value);
  writeBytes(os, t.flags);
  writeBytes(os, t.unused);

  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::ENIT &t, std::size_t /*size*/) {
  readBytes(is, t.value);
  readBytes(is, t.flags);
  readBytes(is, t.unused);
  return is;
}

// ENIT_ENCH specialization
template<>
uint16_t ENIT_ENCH::size() const {
  return 3u * 4u + 1u + 3u * 1u;
}

template<>
std::ostream &raw::write(std::ostream &os,
                         const raw::ENIT_ENCH &t,
                         std::size_t /*size*/) {
  writeBytes(os, t.type);
  writeBytes(os, t.chargeAmount);
  writeBytes(os, t.chargeCost);
  writeBytes(os, t.noAutoCalculate);
  writeBytes(os, t.unused);

  return os;
}

template<>
std::istream &raw::read(std::istream &is,
                        raw::ENIT_ENCH &t,
                        std::size_t /*size*/) {
  readBytes(is, t.type);
  readBytes(is, t.chargeAmount);
  readBytes(is, t.chargeCost);
  readBytes(is, t.noAutoCalculate);
  readBytes(is, t.unused);

  return is;
}

// EFIT specialization
template<>
uint16_t EFIT::size() const {
  return 5u * 4u + sizeof(ActorValue);
}

template<>
std::ostream &raw::write(std::ostream &os,
                         const raw::EFIT &t,
                         std::size_t /*size*/) {
  writeBytes(os, t.efid);
  writeBytes(os, t.magnitude);
  writeBytes(os, t.area);
  writeBytes(os, t.duration);
  writeBytes(os, t.type);
  writeBytes(os, t.avIndex);

  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::EFIT &t, std::size_t /*size*/) {
  readBytes(is, t.efid);
  readBytes(is, t.magnitude);
  readBytes(is, t.area);
  readBytes(is, t.duration);
  readBytes(is, t.type);
  readBytes(is, t.avIndex);

  return is;
}

// SCIT specialization
template<>
uint16_t SCIT::size() const {
  return sizeof(FormId) + sizeof(MagicSchool) + 8u;
}

template<>
std::ostream &raw::write(std::ostream &os,
                         const raw::SCIT &t,
                         std::size_t /*size*/) {
  writeBytes(os, t.id);
  writeBytes(os, t.school);
  writeBytes(os, t.visualEffect);
  writeBytes(os, t.flags);
  writeBytes(os, t.unused);

  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::SCIT &t, std::size_t /*size*/) {
  readBytes(is, t.id);
  readBytes(is, t.school);
  readBytes(is, t.visualEffect);
  readBytes(is, t.flags);
  readBytes(is, t.unused);

  return is;
}

} // namespace record;