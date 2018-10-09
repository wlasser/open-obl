#include "io/string.hpp"

std::string io::readBzString(std::istream &s) {
  // len includes the null-terminator, which needs to be read, but reading len
  // characters gives a string with an internal null which needs to be popped.
  // This is faster than skipping the null or constructing from a char[]
  // (which stops at the first null).
  uint8_t len{0};
  s.read(reinterpret_cast<char *>(&len), 1);
  if (len <= 1u) return "";
  std::string str(len, '\0');
  s.read(str.data(), len);
  str.pop_back();
  return str;
}

std::string io::readBString(std::istream &s) {
  // No null-terminator this time
  uint8_t len{0};
  s.read(reinterpret_cast<char *>(&len), 1);
  if (len < 1u) return "";
  std::string str(len, '\0');
  s.read(str.data(), len);
  return str;
}
