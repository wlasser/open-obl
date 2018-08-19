#ifndef OPENOBLIVION_NIF_VERSIONABLE_HPP
#define OPENOBLIVION_NIF_VERSIONABLE_HPP

#include "io/read_bytes.hpp"
#include <array>
#include <optional>
#include <stdexcept>

namespace nif {

using Version = uint32_t;

constexpr std::size_t chunkLength(uint8_t chunk) {
  if (chunk < 10) return 1;
  else if (chunk < 100) return 2;
  else return 3;
}

constexpr std::size_t versionLength(Version ver) {
  return 3 + chunkLength(static_cast<uint8_t>((ver & 0xff000000) >> 24))
      + chunkLength(static_cast<uint8_t>((ver & 0x00ff0000) >> 16))
      + chunkLength(static_cast<uint8_t>((ver & 0x0000ff00) >> 8))
      + chunkLength(static_cast<uint8_t>((ver & 0x000000ff)));
}

template<Version ver>
const char *verOf() {
  // Since we have to use a static, this computation only needs to be done once
  static bool done = false;
  static std::array<char, versionLength(ver) + 1> v{};
  if (done) return v.data();

  // Current position in v
  std::size_t pos = 0;
  // For each chunk
  for (int i = 0; i < 4; ++i) {
    // Grab the chunk
    int shift = 8 * (3 - i);
    auto chunk = static_cast<uint8_t>((ver & (0xff << shift)) >> shift);

    // Populate v with the decimal digits of the chunk. Since we're only dealing
    // with individual bytes this is simpler to just unroll into the three
    // length cases
    if (chunk < 10) {
      v[pos] = '0' + chunk;
      ++pos;
    } else if (chunk < 100) {
      v[pos] = '0' + (chunk / 10);
      v[pos + 1] = '0' + (chunk % 10);
      pos += 2;
    } else {
      v[pos] = '0' + (chunk / 100);
      v[pos + 1] = '0' + (chunk % 100) / 10;
      v[pos + 2] = '0' + (chunk % 10);
      pos += 3;
    }
    // Don't place a trailing separator
    if (i == 3) break;
    v[pos] = '.';
    ++pos;
  }

  done = true;
  return v.data();
}

class Versionable {
 public:

  static const Version Unbounded = 0xffffffff;

  const Version version;

 protected:

  template<class T, Version ver1, Version ver2>
  class VersionOptional {
   private:
    const Version version;
    std::optional<T> opt;
    const std::optional<Version> lowerBound =
        (ver1 == Unbounded ? std::nullopt : std::make_optional(ver1));
    const std::optional<Version> upperBound =
        (ver2 == Unbounded ? std::nullopt : std::make_optional(ver2));

    constexpr bool verify(Version version) const {
      return (ver1 ? (ver1 <= version) : true)
          && (ver2 ? (version <= ver2) : true);
    }

   public:

    explicit constexpr VersionOptional(Version version) noexcept :
        version(version) {
      if (verify(version)) opt = std::make_optional<T>();
      else opt = std::nullopt;
    }

    constexpr VersionOptional(Version version, T &&value) :
        version(version) {
      if (verify(version)) opt = std::make_optional(value);
      else opt = std::nullopt;
    }

    VersionOptional &operator=(T &&value) {
      if (verify(version)) opt = value;
      return *this;
    }

    constexpr T &value() &{
      if (verify(version)) return opt.value();
      else throw std::bad_optional_access();
    }
    static const int32_t Null = -1;
    constexpr const T &value() const &{
      if (verify(version)) return opt.value();
      else throw std::bad_optional_access();
    }

    constexpr T &&value() &&{
      if (verify(version)) return opt.value();
      else throw std::bad_optional_access();
    }

    constexpr const T &&value() const &&{
      if (verify(version)) return opt.value();
      else throw std::bad_optional_access();
    }

    constexpr explicit operator bool() const noexcept {
      return verify(version);
    }

    constexpr bool has_value() const noexcept {
      return verify(version);
    }

    constexpr const T *operator->() const {
      return opt.operator->();
    }

    constexpr T *operator->() {
      return opt.operator->();
    }

    constexpr const T &operator*() const &{
      return opt.operator*();
    }

    constexpr T &operator*() &{
      return opt.operator*();
    }

    constexpr const T &&operator*() const &&{
      return opt.operator*();
    }

    constexpr T &&operator*() &&{
      return opt.operator*();
    }

    friend std::istream &operator>>(std::istream &is,
                                    VersionOptional<T, ver1, ver2> &t) {
      if (t) is >> t.value();
      return is;
    }
  }; // class VersionOptional

  explicit Versionable(Version version) : version(version) {}

  template<class T, Version ver1, Version ver2>
  friend std::istream &operator>>(std::istream &, T &);

  template<class T, Version ver1, Version ver2>
  friend void ::io::readBytes(std::istream &,
                              VersionOptional<T, ver1, ver2> &);
};

constexpr Version verOf(const char *str, std::size_t size) {
  Version version = 0;

  // Current position in str
  std::size_t pos = 0;

  // For each chunk
  for (auto i = 0; i <= 3; ++i) {
    uint8_t value = 0;

    for (; !(str[pos] == '.' || (i == 3 && pos >= size)); ++pos) {
      if (pos >= size) {
        throw std::out_of_range("incomplete version");
      }
      char c = str[pos];
      if (c < 0x30 || c > 0x39) {
        throw std::range_error("version must contain digits");
      }
      int newValue = 10 * value + c - 0x30;
      if (newValue > std::numeric_limits<uint8_t>::max()) {
        throw std::range_error("version chunk value must be < 255");
      }
      value = static_cast<uint8_t>(newValue);
    }
    // Add to the version
    version = (version << 8) + value;
    // Increase offset to compensate for '.'
    pos += 1;
  }

  return version;
}

namespace literals {

constexpr Version operator ""_ver(const char *str, std::size_t size) {
  return verOf(str, size);
}

} // namespace literal
} // namespace nif

namespace io {

template<class T, nif::Version ver1, nif::Version ver2>
void readBytes(std::istream &is,
               nif::Versionable::VersionOptional<T, ver1, ver2> &t) {
  if (t) io::readBytes(is, t.value());
}

} // namespace io

#endif // OPENOBLIVION_NIF_VERSIONABLE_HPP
