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

void BinaryIo<BaseId>::writeBytes(std::ostream &os, const BaseId &data) {
  BinaryIo<FormId>::writeBytes(os, data.mId);
}

void BinaryIo<BaseId>::readBytes(std::istream &is, BaseId &data) {
  BinaryIo<FormId>::readBytes(is, data.mId);
}

void BinaryIo<RefId>::writeBytes(std::ostream &os, const RefId &data) {
  BinaryIo<FormId>::writeBytes(os, data.mId);
}

void BinaryIo<RefId>::readBytes(std::istream &is, RefId &data) {
  BinaryIo<FormId>::readBytes(is, data.mId);
}

} // namespace io