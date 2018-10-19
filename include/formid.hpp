#ifndef OPENOBLIVION_FORMID_HPP
#define OPENOBLIVION_FORMID_HPP

#include <cstdint>
#include <string>

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

std::string formIdString(FormId formID) noexcept;

class BaseId;
class RefId;

namespace io {

void readBytes(std::istream &, BaseId &);
void writeBytes(std::ostream &, const BaseId &);
void readBytes(std::istream &, RefId &);
void writeBytes(std::ostream &, const RefId &);

}

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

  friend void io::readBytes(std::istream &is, BaseId &baseId);
  friend void io::writeBytes(std::ostream &os, const BaseId &baseId);

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

  friend void io::readBytes(std::istream &is, RefId &refId);
  friend void io::writeBytes(std::ostream &os, const RefId &refId);

 private:
  FormId mId{};
};

inline bool operator==(const RefId &lhs, const RefId &rhs) {
  return static_cast<FormId>(lhs) == static_cast<FormId>(rhs);
}

inline bool operator!=(const RefId &lhs, const RefId &rhs) {
  return !(lhs == rhs);
}

namespace std {

template<>
struct hash<BaseId> {
  size_t operator()(const BaseId &id) const {
    return hash<FormId>{}(static_cast<FormId>(id));
  }
};

template<>
struct hash<RefId> {
  size_t operator()(const RefId &id) const {
    return hash<FormId>{}(static_cast<FormId>(id));
  }
};

} // namespace std

#endif // OPENOBLIVION_FORMID_HPP
