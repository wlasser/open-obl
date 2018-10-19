#include "io/read_bytes.hpp"
#include "io/write_bytes.hpp"
#include "formid.hpp"

std::string formIdString(FormId formID) noexcept {
  static const char *table = "0123456789abcdef";
  std::string s(8, '0');
  for (int i = 0; i < 8; ++i) {
    s[7 - i] = table[formID & 0xf];
    formID >>= 4;
  }
  return s;
}

namespace io {

void readBytes(std::istream &is, BaseId &baseId) {
  readBytes(is, baseId.mId);
}

void writeBytes(std::ostream &os, const BaseId &baseId) {
  writeBytes(os, baseId.mId);
}

void readBytes(std::istream &is, RefId &refId) {
  readBytes(is, refId.mId);
}

void writeBytes(std::ostream &os, const RefId &refId) {
  writeBytes(os, refId.mId);
}

} // namespace io