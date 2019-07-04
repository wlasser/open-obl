#include "record/formid.hpp"
#include <ostream>

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

std::ostream &operator<<(std::ostream &os, BaseId baseId) {
  return os << "0x" << baseId.string();
}

std::ostream &operator<<(std::ostream &os, RefId refId) {
  return os << "0x" << refId.string();
}

} // namespace oo
