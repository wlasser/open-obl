#include "io/read_bytes.hpp"
#include "io/write_bytes.hpp"
#include "record/formid.hpp"

std::string formIdString(FormId formId) noexcept {
  static const char *table = "0123456789abcdef";
  std::string s(8, '0');
  for (int i = 0; i < 8; ++i) {
    s[7 - i] = table[formId & 0xf];
    formId >>= 4;
  }
  return s;
}

void *encodeFormId(FormId formId) noexcept {
  static_assert(sizeof(void *) >= 4);
  std::uintptr_t bits{formId};
  return reinterpret_cast<void *>(bits);
}

FormId decodeFormId(void *ptr) noexcept {
  static_assert(sizeof(void *) >= 4);
  const auto bits{reinterpret_cast<std::uintptr_t>(ptr)};
  return static_cast<FormId>(bits & 0xffffffff);
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