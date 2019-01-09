#ifndef OPENOBLIVION_MAGIC_EFFECTS_HPP
#define OPENOBLIVION_MAGIC_EFFECTS_HPP

#include <array>
#include <cstdint>

namespace oo {

using EffectId = std::array<char, 4>;

enum class MagicSchool : uint32_t {
  Alteration = 0,
  Conjuration = 1,
  Destruction = 2,
  Illusion = 3,
  Mysticism = 4,
  Restoration = 5
};

} // namespace oo

#endif // OPENOBLIVION_MAGIC_EFFECTS_HPP
