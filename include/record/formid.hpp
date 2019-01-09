#ifndef OPENOBLIVION_FORMID_HPP
#define OPENOBLIVION_FORMID_HPP

#include "io/io.hpp"
#include <cstdint>
#include <iomanip>
#include <string>

namespace oo {

// Every record in the game belongs to either the set of all *base records*, or
// the set of all *reference records*. A base record represents the abstract
// concept of a particular thing, and acts as a prototype for the construction
// of concrete realisations of that thing. Reference records are these
// concrete realisations. For instance, every iron sword placed in the game
// world has a different reference record, but all have a base record in common
// which represents the abstract ideal 'iron sword'.
// A *FormId* is a 4 byte number used to identify records in the game. There are
// injective maps
// b: {FormIds} -> {Base records},
// r: {FormIds} -> {Reference records},
// from the set of FormIds to the sets of base/reference records which
// uniquely associate a base/reference record to a FormId. However, there is
// *no* map {FormIds} -> {Base records & Reference records} into the union;
// there may exist a base record and a reference record with the same
// numerical FormId.
// The difference between a base record's FormId and a reference records' FormId
// is therefore crucial, and hence they are represented by different types:
// *BaseId* and *RefId* respectively.

using FormId = uint32_t;
using IRef = uint32_t;

// Lightweight construction of a hexadecimal string representation of formId.
std::string formIdString(FormId formId) noexcept;

// The bullet api allows storing two `int`s and a `void *` as user data in
// collision objects, but it is more convenient to store a FormId. Since it
// would be UB to cast a FormId with large mod index directly to an int, we
// pretend the FormId is an actual pointer and put it in the lower 4 bytes of
// the void *.
void *encodeFormId(FormId formId) noexcept;
FormId decodeFormId(void *ptr) noexcept;

class BaseId;
class RefId;

} // namespace oo

namespace io {

template<>
struct BinaryIo<oo::BaseId> {
  static void writeBytes(std::ostream &os, const oo::BaseId &data);
  static void readBytes(std::istream &is, oo::BaseId &data);
};

template<>
struct BinaryIo<oo::RefId> {
  static void writeBytes(std::ostream &os, const oo::RefId &data);
  static void readBytes(std::istream &is, oo::RefId &data);
};

} // namespace io

namespace oo {

class BaseId {
 public:
  constexpr BaseId() noexcept : mId{} {}

  constexpr explicit BaseId(FormId formId) noexcept : mId{formId} {}

  constexpr explicit operator FormId() const noexcept {
    return mId;
  }

  std::string string() const noexcept {
    return formIdString(static_cast<FormId>(*this));
  }

  friend io::BinaryIo<BaseId>;

  friend std::ostream &operator<<(std::ostream &os, BaseId baseId) {
    os << "0x" << std::hex << std::setfill('0') << std::setw(8) << baseId.mId;
    return os;
  }

 private:
  FormId mId{};
};

inline bool operator==(const BaseId &lhs, const BaseId &rhs) {
  return static_cast<FormId>(lhs) == static_cast<FormId>(rhs);
}

inline bool operator!=(const BaseId &lhs, const BaseId &rhs) {
  return !(lhs == rhs);
}

class RefId {
 public:
  constexpr RefId() noexcept : mId{} {}

  constexpr explicit RefId(FormId formId) noexcept : mId{formId} {}

  constexpr explicit operator FormId() const noexcept {
    return mId;
  }

  std::string string() const noexcept {
    return formIdString(static_cast<FormId>(*this));
  }

  friend io::BinaryIo<RefId>;

  friend std::ostream &operator<<(std::ostream &os, RefId refId) {
    os << "0x" << std::hex << std::setfill('0') << std::setw(8) << refId.mId;
    return os;
  }

 private:
  FormId mId{};
};

inline bool operator==(const RefId &lhs, const RefId &rhs) {
  return static_cast<FormId>(lhs) == static_cast<FormId>(rhs);
}

inline bool operator!=(const RefId &lhs, const RefId &rhs) {
  return !(lhs == rhs);
}

} // namespace oo

namespace std {

template<>
struct hash<oo::BaseId> {
  size_t operator()(const oo::BaseId &id) const {
    return hash<oo::FormId>{}(static_cast<oo::FormId>(id));
  }
};

template<>
struct hash<oo::RefId> {
  size_t operator()(const oo::RefId &id) const {
    return hash<oo::FormId>{}(static_cast<oo::FormId>(id));
  }
};

} // namespace std

#endif // OPENOBLIVION_FORMID_HPP
