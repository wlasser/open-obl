#include "record/formid.hpp"

namespace oo {

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

} // namespace oo

namespace io {

void BinaryIo<oo::BaseId>::writeBytes(std::ostream &os,
                                      const oo::BaseId &data) {
  BinaryIo<oo::FormId>::writeBytes(os, data.mId);
}

void BinaryIo<oo::BaseId>::readBytes(std::istream &is, oo::BaseId &data) {
  BinaryIo<oo::FormId>::readBytes(is, data.mId);
}

void BinaryIo<oo::RefId>::writeBytes(std::ostream &os, const oo::RefId &data) {
  BinaryIo<oo::FormId>::writeBytes(os, data.mId);
}

void BinaryIo<oo::RefId>::readBytes(std::istream &is, oo::RefId &data) {
  BinaryIo<oo::FormId>::readBytes(is, data.mId);
}

} // namespace io