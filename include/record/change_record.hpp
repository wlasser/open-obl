#ifndef OPENOBLIVION_CHANGE_RECORD_HPP
#define OPENOBLIVION_CHANGE_RECORD_HPP

#include "io/io.hpp"
#include "record/exceptions.hpp"
#include "record/formid.hpp"
#include <boost/mp11.hpp>
#include <array>
#include <bitset>
#include <numeric>

namespace record {

/// Trait defining which bits of the flags of a change record correspond to the
/// presence of which change subrecords.
/// This should be specialized for each `ChrRecord`. It should provide a
/// static constant `length` of type `std::size_t` and a static constant `value`
/// of type `std::array<std::size_t, length>`. The value of `length` must be
/// greater than or equal to the number of component change subrecords of `R`.
/// The `i`-th entry of `value` shall be the index of the bit in the change
/// record whose presence signifies the presence of the `i`-th component change
/// subrecord of `R`.
template<class R> struct ChrRecordBits;

/// Helper variable template for `ChrRecordBits<class R>`.
template<class R>
constexpr static inline auto ChrRecordBits_v{ChrRecordBits<R>::value};

/// Trait defining the numerical type of a change record.
/// This should be specialized for each `ChrRecord` and should provide a static
/// constant `value` of type `uint8_t` such that the map
/// `R |-> ChrRecordType<R>::value` is injective.
template<class R> struct ChrRecordType;

/// Helper variable template for `ChrRecordType<class R>`.
template<class R>
constexpr static inline auto ChrRecordType_v{ChrRecordType<R>::value};

/// A change record representing a diff against the `Record` type `R` with
/// change subrecords given by `Subrecords`.
///
/// Each change record type consists of multiple component change subrecords.
/// The set of all change subrecords that could appear for a given change record
/// type is fixed and given by the `Subrecords`. The order in which the change
/// subrecords appear when the change record is serialized is also fixed, and is
/// the same order as they appear in `Subrecords`, but the subset of the change
/// subrecords that are present is variable and determined by `flags`.
/// Specifically, if the `i`-th bit of the `flags` is set then the `j`-th
/// change subrecord is present, where `j` is the value at the `i`-th entry of
/// `ChrRecordBits<Self>::value`, and `Self` is the appropriate specialization
/// of this class.
template<class R, class ... Subrecords>
struct ChrRecord {
  oo::RefId refId{};
  std::bitset<32u> flags{};
  std::tuple<Subrecords...> components;

  constexpr explicit ChrRecord(oo::RefId refId, uint32_t flags) noexcept
      : refId(refId), flags(flags) {}
};

/// Read a change subrecord from a stream.
/// `tailSize` is the size of the change subrecord to read plus the size of any
/// change subrecords after it.
/// \remark This should be specialized for each change subrecord.
template<class Subrec> std::istream &
readSubrecord(std::istream &is, Subrec &subrec, uint16_t tailSize) = delete;

/// Write a change subrecord to a stream.
/// \remark This should be specialized for each change subrecord.
template<class Subrec> std::ostream &
writeSubrecord(std::ostream &os, const Subrec &subrec) = delete;

/// A change subrecord must satisfy the following concept.
/// ```cpp
/// template<class R> concept ChrSubrecord = std::Semiregular<R>
///     && requires(R &x, std::istream &is, uint16_t size) {
///       { record::readSubrecord<R>(is, x, size); } -> std::istream &;
///     } && requires(const R &x, std::ostream &os) {
///       { record::writeSubrecord<R>(os, x); } -> std::ostream &;
///       { x.size() } -> uint16_t;
///     };
/// ```
// C++20: Make ChrSubrecord an actual concept.

/// Read a change record from a stream.
template<class Rec, class = std::enable_if_t<ChrRecordType<Rec>::value != 0u>>
Rec readRecord(std::istream &is) {
  // RefId of record to diff against, or a new record if top byte is 0xff.
  oo::RefId refId{};
  io::readBytes(is, refId);

  // Type should match our hardcoded one.
  uint8_t type{};
  io::readBytes(is, type);
  constexpr uint8_t expectedType{ChrRecordType_v<Rec>};
  if (type != expectedType) {
    throw RecordNotFoundError(std::to_string(expectedType),
                              std::to_string(type));
  }

  uint32_t flags{};
  io::readBytes(is, flags);

  // Don't care about the version, it's always 125.
  {
    uint8_t version{};
    io::readBytes(is, version);
  }

  // Total size of all change subrecords present.
  uint16_t size{};
  io::readBytes(is, size);

  Rec chrRec(refId, flags);

  // We know that the components are in the order that they should be read, so
  // loop over them and skip any that the flags say aren't present.
  std::size_t index{0u};
  boost::mp11::tuple_for_each(chrRec.components, [&](auto &component) {
    const std::size_t bit{ChrRecordBits_v<Rec>[index++]};
    if (!chrRec.flags[bit]) return;
    record::readSubrecord(is, component, size);
    size -= component.size();
  });

  return chrRec;
}

/// Write a change record to a stream.
template<class Rec, class = std::enable_if_t<ChrRecordType<Rec>::value != 0u>>
void writeRecord(std::ostream &os, const Rec &chrRec) {
  io::writeBytes(os, chrRec.refId);
  io::writeBytes(os, ChrRecordType_v<Rec>);
  io::writeBytes(os, chrRec.flags);
  io::writeBytes(os, static_cast<uint8_t>(125u));

  const uint16_t size{std::accumulate(chrRec.components.begin(),
                                      chrRec.components.end(),
                                      [](uint16_t s, const auto &component) {
                                        return s + component.size();
                                      })};
  io::writeBytes(os, size);

  std::size_t index{0u};
  boost::mp11::tuple_for_each(chrRec.components, [&](const auto &component) {
    const std::size_t bit{ChrRecordBits_v<Rec>[index++]};
    if (!chrRec.flags[bit]) return;
    record::writeSubrecord(os, component);
  });
}

} // namespace record

#endif // OPENOBLIVION_CHANGE_RECORD_HPP
