#ifndef OPENOBLIVION_REC_OF_HPP
#define OPENOBLIVION_REC_OF_HPP

#include <array>
#include <cstdint>

namespace record {

// Convert a 4-character string literal to a little-endian integer
inline constexpr uint32_t recOf(const char *r, std::size_t size) {
  return size != 4 ? 0u : (static_cast<uint8_t>(r[0])
      + (static_cast<uint8_t>(r[1]) << 8u)
      + (static_cast<uint8_t>(r[2]) << 16u)
      + (static_cast<uint8_t>(r[3]) << 24u));
}

inline constexpr uint32_t recOf(const std::array<char, 4> &r) {
  return recOf(r.data(), 4);
}

inline constexpr uint32_t operator "" _rec(const char *r, std::size_t size) {
  return recOf(r, size);
}

// Convert back from a integer to a 4-character string literal
template<uint32_t t>
const char *recOf() {
  static const char r[] = {
      static_cast<const char>(t & 0xffu),
      static_cast<const char>((t & (0xffu << 8u)) >> 8u),
      static_cast<const char>((t & (0xffu << 16u)) >> 16u),
      static_cast<const char>((t & (0x0ffu << 24u)) >> 24u),
      '\0'
  };
  return r;
}

} // namespace record

#endif //OPENOBLIVION_REC_OF_HPP
