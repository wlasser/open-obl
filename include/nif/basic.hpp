#ifndef OPENOBLIVION_NIF_BASIC_HPP
#define OPENOBLIVION_NIF_BASIC_HPP

#include "nif/versionable.hpp"
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <sstream>

namespace nif {
namespace basic {

// nifxml says '32-bit from 4.0.0.2, and 8-bit 4.1.0.1 on'. Oblivion uses nif
// files with ver = 3.3.0.13 and ver >= 10.0.1.2, so assuming the first 'from'
// also means 'from ... onwards', bool can be assumed to always be 8 bits.
// sizeof(bool) is not required by the standard to be 1, but we will assume it.
static_assert(sizeof(bool) == 1);
using Bool = bool;

enum class Byte : uint8_t {};
using UInt = uint32_t;

// Little endian is assumed throughout most of the code base, so it probably
// isn't worth trying to get it right here (using Boost/Endian). These macros
// work on GCC and Clang, sorry MSVC.
// TODO: In C++20, use std::endian
static_assert(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__);
using ULittle32 = uint32_t;

using UShort = uint16_t;
using Int = int32_t;
using Short = int16_t;
using BlockTypeIndex = uint16_t;
using Char = char;
using FileVersion = Version;
using Flags = uint16_t;
using Float = float;

// Newline-terminated string
struct HeaderString {
  std::string str;
};

// Newline-terminated string
struct LineString {
  std::string str;
};

// Points to an object further up in the hierarchy
template<class T>
class Ptr {
 private:
  int32_t val{};
  friend std::istream &operator>>(std::istream &is, Ptr<T> &t) {
    io::readBytes(is, t.val);
    return is;
  }
};

// Points to an object further down in the hierarchy.
// Can be null.
template<class T>
class Ref {
 private:
  int32_t val{};
 public:
  static const int32_t Null = -1;
  friend std::istream &operator>>(std::istream &is, Ref<T> &t) {
    io::readBytes(is, t.val);
    return is;
  }
};
enum class StringOffset : uint32_t;
enum class StringIndex : uint32_t;

std::istream &operator>>(std::istream &is, HeaderString &t);
std::istream &operator>>(std::istream &is, LineString &t);

} // namespace basic
} // namespace nif

#endif // OPENOBLIVION_NIF_BASIC_HPP
