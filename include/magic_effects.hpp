#ifndef MAGIC_EFFECTS_HPP
#define MAGIC_EFFECTS_HPP

#include <cstdint>
#include <array>

using EffectID = std::array<char, 4>;

enum class MagicSchool : uint32_t {
  Alteration = 0,
  Conjuration = 1,
  Destruction = 2,
  Illusion = 3,
  Mysticism = 4,
  Restoration = 5
};

#endif // MAGIC_EFFECTS_HPP
