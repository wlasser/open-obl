#ifndef OPENOBLIVION_ATTRIBUTES_HPP
#define OPENOBLIVION_ATTRIBUTES_HPP

#include <cstdint>

namespace oo {

enum class Attribute : uint32_t {
  Strength = 0u,
  Intelligence = 1u,
  Willpower = 2u,
  Agility = 3u,
  Speed = 4u,
  Endurance = 5u,
  Personality = 6u,
  Luck = 7u
};

} // namespace oo

#endif // OPENOBLIVION_ATTRIBUTES_HPP