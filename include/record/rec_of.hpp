#ifndef OPENOBL_RECORD_REC_OF_HPP
#define OPENOBL_RECORD_REC_OF_HPP

#include <array>
#include <cstdint>
#include <string_view>

namespace record {

/// Interpret the bytes of a string as a little-endian integer.
/// If `sv.length() != 4`, this function returns 0.
/// For example, providing the function `"char" == {0x63, 0x68, 0x61, 0x72}`
/// returns the integer `0x72616863`.
inline constexpr uint32_t recOf(std::string_view sv) noexcept {
  return sv.length() != 4 ? 0u : (static_cast<uint8_t>(sv[0])
      + (static_cast<uint8_t>(sv[1]) << 8u)
      + (static_cast<uint8_t>(sv[2]) << 16u)
      + (static_cast<uint8_t>(sv[3]) << 24u));
}

/// \overload inline constexpr uint32_t recOf(std::string_view sv) noexcept
inline constexpr uint32_t recOf(const std::array<char, 4> &r) noexcept {
  return recOf(std::string_view(r.data(), 4));
}

inline namespace literals {

/// UDL that calls recOf on the passed string literal.
inline constexpr uint32_t operator "" _rec(const char *r, std::size_t size) {
  return recOf(std::string_view(r, size));
}

} // namespace literals

/// Interpret the bytes of a little-endian integer as a string.
/// This overload is the inverse of the overload taking a std::string_view, in
/// the sense that `recOf<recOf(sv)>() == sv` for all `std::string_view sv`
/// such that `sv.length() == 4` and `recOf(recOf<t>()) == t` for all
/// `uint32_t t`.
template<uint32_t t>
std::string_view recOf() noexcept {
  constexpr static char r[]{
      static_cast<char>(t & 0xffu),
      static_cast<char>((t & (0xffu << 8u)) >> 8u),
      static_cast<char>((t & (0xffu << 16u)) >> 16u),
      static_cast<char>((t & (0x0ffu << 24u)) >> 24u),
      '\0'
  };
  return std::string_view(r);
}

/// \overload recOf()
inline std::string recOf(uint32_t t) noexcept {
  std::array<char, 5> r{
      static_cast<char>(t & 0xffu),
      static_cast<char>((t & (0xffu << 8u)) >> 8u),
      static_cast<char>((t & (0xffu << 16u)) >> 16u),
      static_cast<char>((t & (0x0ffu << 24u)) >> 24u),
      '\0'
  };
  return std::string(r.data());
}

} // namespace record

#endif //OPENOBL_RECORD_REC_OF_HPP
