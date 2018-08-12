#ifndef ATTRIBUTES_HPP
#define ATTRIBUTES_HPP

#include <cstdint>

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

#endif /* ATTRIBUTES_HPP */