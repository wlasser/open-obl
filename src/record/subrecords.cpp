#include "io/io.hpp"
#include "record/io.hpp"
#include "record/subrecords.hpp"
#include <istream>
#include <ostream>

namespace record {

// SPIT specialization
template<>
uint16_t SPIT::size() const {
  return 16u;
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::SPIT &t, std::size_t /*size*/) {
  io::writeBytes(os, t.type);
  io::writeBytes(os, t.cost);
  io::writeBytes(os, t.level);
  raw::write(os, t.flags, 0);

  return os;
}

template<>
std::istream &
raw::read(std::istream &is, raw::SPIT &t, std::size_t /*size*/) {
  io::readBytes(is, t.type);
  io::readBytes(is, t.cost);
  io::readBytes(is, t.level);
  raw::read(is, t.flags, 0);

  return is;
}

// VTXT specialization
template<>
uint16_t VTXT::size() const {
  return data.points.size() * 8u;
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::VTXT &t, std::size_t /*size*/) {
  for (const auto &p : t.points) {
    io::writeBytes(os, p.position);
    io::writeBytes(os, p.unused);
    io::writeBytes(os, p.opacity);
  }

  return os;
}

template<>
std::istream &
raw::read(std::istream &is, raw::VTXT &t, std::size_t size) {
  for (std::size_t i = 0; i < size / 8u; ++i) {
    auto &p{t.points.emplace_back()};
    io::readBytes(is, p.position);
    io::readBytes(is, p.unused);
    io::readBytes(is, p.opacity);
  }

  return is;
}

// XSED specialization
template<>
uint16_t XSED::size() const {
  return data.size;
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::XSED &t, std::size_t size) {
  for (auto i = 0; i < size; ++i) {
    os.put(0);
  }
  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::XSED &t, std::size_t size) {
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
std::ostream &
raw::write(std::ostream &os, const raw::XRGD &t, std::size_t /*size*/) {
  for (const auto byte : t.bytes) {
    io::writeBytes(os, byte);
  }
  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::XRGD &t, std::size_t size) {
  const std::size_t length = size / 1u;
  io::readBytes(is, t.bytes, length);
  return is;
}

// XLOC specicalization
template<>
uint16_t XLOC::size() const {
  return 4u + sizeof(oo::FormId) + 4u;
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::XLOC &t, std::size_t /*size*/) {
  io::writeBytes(os, t.lockLevel);
  io::writeBytes(os, t.key);
  //io::writeBytes(os, t.unused);
  raw::write(os, t.flags, 0);
  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::XLOC &t, std::size_t size) {
  io::readBytes(is, t.lockLevel);
  io::readBytes(is, t.key);
  if (size == 16) io::readBytes(is, t.unused);
  raw::read(is, t.flags, 0);
  return is;
}

// XESP specialization
template<>
uint16_t XESP::size() const {
  return sizeof(oo::FormId) + sizeof(raw::XESP::Flag);
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::XESP &t, std::size_t /*size*/) {
  io::writeBytes(os, t.parent);
  raw::write(os, t.flags, 0);
  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::XESP &t, std::size_t /*size*/) {
  io::readBytes(is, t.parent);
  raw::read(is, t.flags, 0);
  return is;
}

// XCLR specialization
template<>
uint16_t XCLR::size() const {
  return sizeof(oo::FormId) * data.regions.size();
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::XCLR &t, std::size_t /*size*/) {
  for (const auto &region : t.regions) {
    io::writeBytes(os, region);
  }
  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::XCLR &t, std::size_t size) {
  const std::size_t length = size / sizeof(oo::FormId);
  io::readBytes(is, t.regions, length);
  return is;
}

// HNAM_LTEX specialization
template<>
uint16_t HNAM_LTEX::size() const {
  return 3u * 1u;
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::HNAM_LTEX &t, std::size_t /*size*/) {
  io::writeBytes(os, t.type);
  io::writeBytes(os, t.friction);
  io::writeBytes(os, t.restitution);
  return os;
}

template<>
std::istream &
raw::read(std::istream &is, raw::HNAM_LTEX &t, std::size_t /*size*/) {
  io::readBytes(is, t.type);
  io::readBytes(is, t.friction);
  io::readBytes(is, t.restitution);
  return is;
}

// HNAM specialization
template<>
uint16_t HNAM::size() const {
  return data.hair.size() * sizeof(oo::FormId);
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::HNAM &t, std::size_t /*size*/) {
  for (const auto hair : t.hair) {
    io::writeBytes(os, hair);
  }
  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::HNAM &t, std::size_t size) {
  const std::size_t length = size / sizeof(oo::FormId);
  io::readBytes(is, t.hair, length);
  return is;
}

// ENAM specialization
template<>
uint16_t ENAM::size() const {
  return data.eyes.size() * sizeof(oo::FormId);
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::ENAM &t, std::size_t /*size*/) {
  for (const auto eye : t.eyes) {
    io::writeBytes(os, eye);
  }
  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::ENAM &t, std::size_t size) {
  const std::size_t length = size / sizeof(oo::FormId);
  io::readBytes(is, t.eyes, length);
  return is;
}

// ESCE specialization
template<>
uint16_t ESCE::size() const {
  return data.effects.size() * sizeof(oo::EffectId);
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::ESCE &t, std::size_t /*size*/) {
  for (const auto &effect : t.effects) {
    io::writeBytes(os, effect);
  }
  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::ESCE &t, std::size_t size) {
  const std::size_t length = size / sizeof(oo::EffectId);
  io::readBytes(is, t.effects, length);
  return is;
}

// SNDD specialization
template<>
uint16_t SNDD::size() const {
  return 4u * 1u + 4u;
};

template<>
std::ostream &
raw::write(std::ostream &os, const raw::SNDD &t, std::size_t /*size*/) {
  io::writeBytes(os, t.minAttenuationDistance);
  io::writeBytes(os, t.maxAttenuationDistance);
  io::writeBytes(os, t.frequencyAdjustment);
  io::writeBytes(os, t.unused);
  raw::write(os, t.flags, 0);
  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::SNDD &t, std::size_t /*size*/) {
  io::readBytes(is, t.minAttenuationDistance);
  io::readBytes(is, t.maxAttenuationDistance);
  io::readBytes(is, t.frequencyAdjustment);
  io::readBytes(is, t.unused);
  raw::read(is, t.flags, 0);
  return is;
}

// SNDX specialization
template<>
uint16_t SNDX::size() const {
  return 4u * 1u + 2u * 4u + (data.staticAttenuation ? 4u : 0u)
      + (data.startTime ? 4u : 0u) + (data.stopTime ? 4u : 0u);
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::SNDX &t, std::size_t /*size*/) {
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

template<>
std::istream &raw::read(std::istream &is, raw::SNDX &t, std::size_t size) {
  io::readBytes(is, t.minAttenuationDistance);
  io::readBytes(is, t.maxAttenuationDistance);
  io::readBytes(is, t.frequencyAdjustment);
  io::readBytes(is, t.unused);
  raw::read(is, t.flags, 0);
  io::readBytes(is, t.unusedWord);
  if (size > 12) io::readBytes(is, t.staticAttenuation);
  if (size > 16) io::readBytes(is, t.startTime);
  if (size > 17) io::readBytes(is, t.stopTime);
  return is;
}

// DATA_MGEF specialization
template<>
uint16_t DATA_MGEF::size() const {
  return sizeof(raw::DATA_MGEF::Flag) + 4u + sizeof(oo::MagicSchool)
      + sizeof(oo::ActorValue) + 2u * sizeof(uint16_t) + 4u * sizeof(float)
      + 7u * sizeof(oo::FormId);
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::DATA_MGEF &t, std::size_t /*size*/) {
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

template<>
std::istream &
raw::read(std::istream &is, raw::DATA_MGEF &t, std::size_t size) {
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

// DATA_RACE specialization
template<>
uint16_t DATA_RACE::size() const {
  return sizeof(oo::ActorValue) * 1u * data.skillModifiers.size() + 2u * 1u
      + 4u * 4u + 4u;
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::DATA_RACE &t, std::size_t /*size*/) {
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

// DATA_CLAS specialization
template<>
uint16_t DATA_CLAS::size() const {
  return 4u * 12u + (data.hasTrainingInfo ? 4u : 0u);
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::DATA_CLAS &t, std::size_t /*size*/) {
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

template<>
std::istream &raw::read(std::istream &is, raw::DATA_CLAS &t, std::size_t size) {
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

// DATA_GMST specialization
template<>
uint16_t DATA_GMST::size() const {
  return data.s.size() * 1u;
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::DATA_GMST &t, std::size_t /*size*/) {
  os.write(t.s.data(), t.s.size() * 1);
  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::DATA_GMST &t, std::size_t size) {
  t.s.clear();
  io::readBytes(is, t.s, size);
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
std::ostream &
raw::write(std::ostream &os, const raw::DATA_LIGH &t, std::size_t /*size*/) {
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

template<>
std::istream &raw::read(std::istream &is, raw::DATA_LIGH &t, std::size_t size) {
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

// EFID specialization
template<>
uint16_t EFID::size() const {
  return 4;
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::EFID &t, std::size_t /*size*/) {
  io::writeBytes(os, t);
  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::EFID &t, std::size_t /*size*/) {
  io::readBytes(is, t);
  return is;
}

// OFST specialization
template<>
uint16_t OFST::size() const {
  return 3u * 4u * data.unused.size();
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::OFST &t, std::size_t /*size*/) {
  for (const auto &entry : t.unused) {
    io::writeBytes(os, entry);
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

// OFST_WRLD specialization
template<>
uint16_t OFST_WRLD::size() const {
  return data.entries.size() * 4u;
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::OFST_WRLD &t, std::size_t /*size*/) {
  for (const auto entry : t.entries) io::writeBytes(os, entry);
  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::OFST_WRLD &t, std::size_t size) {
  const std::size_t length = size / 4u;
  io::readBytes(is, t.entries, length);
  return is;
}

// DELE specialization
template<>
uint16_t DELE::size() const {
  return data.size;
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::DELE &/*t*/, std::size_t size) {
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

// MNAM_WRLD specialization
template<>
uint16_t MNAM_WRLD::size() const {
  return 16u;
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::MNAM_WRLD &t, std::size_t /*size*/) {
  io::writeBytes(os, t.width);
  io::writeBytes(os, t.height);
  io::writeBytes(os, t.topLeft.x);
  io::writeBytes(os, t.topLeft.y);
  io::writeBytes(os, t.bottomRight.x);
  io::writeBytes(os, t.bottomRight.y);

  return os;
}

template<>
std::istream &
raw::read(std::istream &is, raw::MNAM_WRLD &t, std::size_t size) {
  io::readBytes(is, t.width);
  io::readBytes(is, t.height);
  io::readBytes(is, t.topLeft.x);
  io::readBytes(is, t.topLeft.y);
  io::readBytes(is, t.bottomRight.x);
  io::readBytes(is, t.bottomRight.y);

  return is;
}

// MODT specialization
template<>
uint16_t MODT::size() const {
  return 3u * 8u * data.records.size();
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::MODT &t, std::size_t /*size*/) {
  for (const auto &record : t.records) {
    io::writeBytes(os, record.ddsHash);
    io::writeBytes(os, record.ddxHash);
    io::writeBytes(os, record.folderHash);
  }
  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::MODT &t, std::size_t size) {
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

// ENIT specialization
template<>
uint16_t ENIT::size() const {
  return 8;
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::ENIT &t, std::size_t /*size*/) {
  io::writeBytes(os, t.value);
  raw::write(os, t.flags, 0);
  io::writeBytes(os, t.unused);

  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::ENIT &t, std::size_t /*size*/) {
  io::readBytes(is, t.value);
  raw::read(is, t.flags, 0);
  io::readBytes(is, t.unused);
  return is;
}

// ENIT_ENCH specialization
template<>
uint16_t ENIT_ENCH::size() const {
  return 3u * 4u + 1u + 3u * 1u;
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::ENIT_ENCH &t, std::size_t /*size*/) {
  io::writeBytes(os, t.type);
  io::writeBytes(os, t.chargeAmount);
  io::writeBytes(os, t.chargeCost);
  io::writeBytes(os, t.noAutoCalculate);
  io::writeBytes(os, t.unused);

  return os;
}

template<>
std::istream &
raw::read(std::istream &is, raw::ENIT_ENCH &t, std::size_t /*size*/) {
  io::readBytes(is, t.type);
  io::readBytes(is, t.chargeAmount);
  io::readBytes(is, t.chargeCost);
  io::readBytes(is, t.noAutoCalculate);
  io::readBytes(is, t.unused);

  return is;
}

// EFIT specialization
template<>
uint16_t EFIT::size() const {
  return 5u * 4u + sizeof(oo::ActorValue);
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::EFIT &t, std::size_t /*size*/) {
  io::writeBytes(os, t.efid);
  io::writeBytes(os, t.magnitude);
  io::writeBytes(os, t.area);
  io::writeBytes(os, t.duration);
  io::writeBytes(os, t.type);
  io::writeBytes(os, t.avIndex);

  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::EFIT &t, std::size_t /*size*/) {
  io::readBytes(is, t.efid);
  io::readBytes(is, t.magnitude);
  io::readBytes(is, t.area);
  io::readBytes(is, t.duration);
  io::readBytes(is, t.type);
  io::readBytes(is, t.avIndex);

  return is;
}

// SCIT specialization
template<>
uint16_t SCIT::size() const {
  return sizeof(oo::FormId) + sizeof(oo::MagicSchool) + 8u;
}

template<>
std::ostream &
raw::write(std::ostream &os, const raw::SCIT &t, std::size_t /*size*/) {
  io::writeBytes(os, t.id);
  io::writeBytes(os, t.school);
  io::writeBytes(os, t.visualEffect);
  raw::write(os, t.flags, 0);
  io::writeBytes(os, t.unused);

  return os;
}

template<>
std::istream &raw::read(std::istream &is, raw::SCIT &t, std::size_t /*size*/) {
  io::readBytes(is, t.id);
  io::readBytes(is, t.school);
  io::readBytes(is, t.visualEffect);
  raw::read(is, t.flags, 0);
  io::readBytes(is, t.unused);

  return is;
}

// ACBS specialization
template<> uint16_t ACBS::size() const {
  return 16u;
}

template<> std::ostream &
raw::write(std::ostream &os, const raw::ACBS &t, std::size_t /*size*/) {
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
raw::read(std::istream &is, raw::ACBS &t, std::size_t /*size*/) {
  raw::read(is, t.flags, 0);
  io::readBytes(is, t.baseSpellPoints);
  io::readBytes(is, t.baseFatigue);
  io::readBytes(is, t.barterGold);
  io::readBytes(is, t.level);
  io::readBytes(is, t.calcMin);
  io::readBytes(is, t.calcMax);

  return is;
}

// AIDT specialization
template<> uint16_t AIDT::size() const {
    return 12u;
}

template<> std::ostream &
raw::write(std::ostream &os, const raw::AIDT &t, std::size_t /*size*/) {
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
raw::read(std::istream &is, raw::AIDT &t, std::size_t /*size*/) {
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

} // namespace record;
