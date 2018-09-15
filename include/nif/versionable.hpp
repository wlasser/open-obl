#ifndef OPENOBLIVION_NIF_VERSIONABLE_HPP
#define OPENOBLIVION_NIF_VERSIONABLE_HPP

#include "io/read_bytes.hpp"
#include <array>
#include <optional>
#include <stdexcept>
#include <variant>

// Nif files are heavily versioned, with different parts appearing only under
// certain version constraints. To supply these version constraints, one derives
// from the Versionable class and wraps any versioned members in a
// VersionOptional. These check the actual runtime version against the version
// constraints and do nothing in cases where the constraints are not satisfied.
namespace nif {
// Version information in nif files is given as a sequence of 4 chunks
// separated by '.', such as '20.0.0.5' or '10.1.0.101'. They are ordered in the
// same way as semver, so that version 'a' is newer/greater than version 'b'
// iff there is some chunk in 'a' that is greater than the corresponding
// chunk in 'b', and every chunk to the left is the same in both 'a'
// and 'b'. Thus '10.1.2.101 > 10.1.2.100 > 10.1.1.200 > 9.5.5.5` and so on.
//
// The version chunks are allowed to be any nonnegative integer less than
// 256, so fit in a single byte. By concatenating the version numbers so that
// the left-most chunk is the most-significant byte one obtains a bijection
// between nif versions and 4-byte integers. For example, '20.0.0.5' becomes
// the integer 0x14000005. Versions are therefore stored with type uint32_t.
using Version = uint32_t;

// Compute the number of digits in the decimal expansion of chunk.
constexpr std::size_t chunkLength(uint8_t chunk) {
  if (chunk < 10) return 1;
  else if (chunk < 100) return 2;
  else return 3;
}

// Compute the number of characters (not including the null-terminator) of the
// string representation of ver.
constexpr std::size_t versionLength(Version ver) {
  return 3 + chunkLength(static_cast<uint8_t>((ver & 0xff000000u) >> 24u))
      + chunkLength(static_cast<uint8_t>((ver & 0x00ff0000u) >> 16u))
      + chunkLength(static_cast<uint8_t>((ver & 0x0000ff00u) >> 8u))
      + chunkLength(static_cast<uint8_t>((ver & 0x000000ffu)));
}

// Return the string representation of ver. To save dynamically allocating
// memory, which would then have to be deleted by the client, a char array
// with static storage duration is constructed and returned. By making this a
// function template, each instantiation has a different static char array,
// preventing conflict.
// TODO: Why is this not std::string verOf(Version ver)
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
    auto shift = static_cast<unsigned int>(8 * (3 - i));
    auto chunk = static_cast<uint8_t>((ver & (0xffu << shift)) >> shift);

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

// Any class representing a versioned component of a nif file should derive from
// this class. It is intended that the derived class have a constructor taking
// a Version used to construct the Versionable. The Version is not marked const
// to facilitate move semantics, although it is not intended to be modified.
class Versionable {
 public:
  // Used in VersionOptional to denote that a lower or upper version requirement
  // is missing.
  static const Version Unbounded = 0xffffffffu;
  // TODO: compound::Header is preventing this from being protected with complicated versioning
  /*const*/ Version version;

 protected:
  // Every versioned component should be wrapped in a VersionOptional, which has
  // two version requirement bounds ver1 and ver2. The VersionOptional must be
  // given a Version on construction (it is intended that the parent
  // Versionable's version be used, though this is not required) which is
  // compared against the version requirement
  // ver1 <= version && version <= ver2
  // on each operation. If ver1 or ver2 is marked as Unbounded, then that
  // constraint is ignored. If the requirement is not satisfied, then the
  // VersionOptional cannot be assigned to or accessed, and is 'inactive'.
  // Attempting to access an inactive VersionOptional throws
  // std::bad_optional_access.
  template<class T, Version ver1, Version ver2>
  class VersionOptional {
   private:
    /*const*/ Version version;
    std::optional<T> opt;
    /*const*/ std::optional<Version> lowerBound =
        (ver1 == Unbounded ? std::nullopt : std::make_optional(ver1));
    /*const*/ std::optional<Version> upperBound =
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

    constexpr VersionOptional(const VersionOptional &other) = default;
    constexpr VersionOptional &operator=(const VersionOptional &other) = default;
    constexpr VersionOptional(VersionOptional &&other) noexcept = default;
    constexpr VersionOptional &operator=(VersionOptional &&other) noexcept = default;

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

  // Sometimes a versioned component is always present, but changes type based
  // on version. This class functions like VersionOptional but presents as type
  // R when the version requirement is satisfied, and as type L otherwise.
  template<class L, class R, Version ver1, Version ver2>
  class VersionEither {
   private:
    /*const*/ Version version;
    std::variant<L, R> var;
    /*const*/ std::optional<Version> lowerBound =
        (ver1 == Unbounded ? std::nullopt : std::make_optional(ver1));
    /*const*/ std::optional<Version> upperBound =
        (ver2 == Unbounded ? std::nullopt : std::make_optional(ver2));

    constexpr bool verify(Version version) const {
      return (ver1 ? (ver1 <= version) : true)
          && (ver2 ? (version <= ver2) : true);
    }

   public:
    explicit constexpr VersionEither(Version version) :
        version(version), var(L()) {
      if (verify(version)) var.template emplace<R>();
      else var.template emplace<L>();
    }

    constexpr VersionEither(const VersionEither &other) = default;
    constexpr VersionEither &operator=(const VersionEither &other) = default;
    constexpr VersionEither(VersionEither &&other) noexcept = default;
    constexpr VersionEither &operator=(VersionEither &&other) noexcept = default;

    constexpr VersionEither(Version version, L &&value) : version(version) {
      if (!verify(version)) var = value;
    }

    constexpr VersionEither(Version version, R &&value) : version(version) {
      if (verify(version)) var = value;
    }

    template<class T>
    VersionEither &operator=(T &&value) {
      if (verify(version)) std::get<R>(var) = std::forward<T>(value);
      else std::get<L>(var) = std::forward<T>(value);
      return *this;
    }

    constexpr explicit operator bool() const noexcept {
      return verify(version);
    }

    template<bool b>
    constexpr auto &value() {
      return std::get<static_cast<std::size_t>(b)>(var);
    }

    template<class T, class FL, class FR>
    constexpr T fold(FL &&fl, FR &&fr) {
      if (verify(version)) {
        return std::invoke(std::forward<FR>(fr), std::get<R>(var));
      } else {
        return std::invoke(std::forward<FL>(fl), std::get<L>(var));
      }
    }

    friend std::istream &operator>>(std::istream &is,
                                    VersionEither<L, R, ver1, ver2> &t) {
      if (t) is >> std::get<R>(t.var);
      else is >> std::get<L>(t.var);
      return is;
    }
  }; // class VersionEither

  explicit Versionable(Version version) : version(version) {}

  constexpr Versionable(const Versionable &other) = default;
  Versionable &operator=(const Versionable &other) = default;
  constexpr Versionable(Versionable &&other) noexcept = default;
  Versionable &operator=(Versionable &&other) noexcept = default;

  template<class T, Version ver1, Version ver2>
  friend std::istream &operator>>(std::istream &, T &);

  template<class T, nif::Version ver1, nif::Version ver2>
  friend void ::io::readBytes(std::istream &,
                              VersionOptional<T, ver1, ver2> &);

  template<class L, class R, nif::Version ver1, nif::Version ver2>
  friend void ::io::readBytes(std::istream &,
                              VersionEither<L, R, ver1, ver2> &);
};

// Convert a version string into its integer representation.
constexpr Version verOf(const char *str, std::size_t size) {
  Version version = 0;

  // Current position in str
  std::size_t pos = 0;

  // For each chunk
  for (int i = 0; i <= 3; ++i) {
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
    version = (version << 8u) + value;
    // Increase offset to compensate for '.'
    pos += 1;
  }

  // If we are not at the end of the string, then the version is too long
  // and must be invalid.
  if (pos <= size) {
    throw std::range_error("version is too long, maybe too many chunks?");
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

template<class L, class R, nif::Version ver1, nif::Version ver2>
void readBytes(std::istream &is,
               nif::Versionable::VersionEither<L, R, ver1, ver2> &t) {
  if (t) io::readBytes(is, t.template value<true>());
  else io::readBytes(is, t.template value<false>());
}

} // namespace io

#endif // OPENOBLIVION_NIF_VERSIONABLE_HPP
