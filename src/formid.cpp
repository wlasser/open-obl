#include "formid.hpp"

std::string formIDString(FormID formID) {
  static const char *table = "0123456789abcdef";
  std::string s(8, '0');
  for (int i = 0; i < 8; ++i) {
    s[7 - i] = table[formID & 0xf];
    formID >>= 4;
  }
  return s;
}