#ifndef OPENOBLIVION_RECORD_SUBRECORD_HPP
#define OPENOBLIVION_RECORD_SUBRECORD_HPP

#include "io/io.hpp"
#include "record/io.hpp"
#include "record/size_of.hpp"
#include "record/rec_of.hpp"
#include <array>
#include <cstdint>
#include <istream>
#include <ostream>
#include <string_view>

namespace record {

/// Wrapper class for subrecords.
/// \tparam T The raw subrecord type to wrap. Must model the
///           std::DefaultConstructible concept.
/// \tparam c An integer representing the subrecord type. If the subrecord type
///           is "ABCD" then `c == recOf("ABCD")`.
template<class T, uint32_t c>
struct Subrecord {
  /// The integer representation of the subrecord type.
  constexpr static inline uint32_t RecordType{c};

  /// Size of the subrecord data when written to disk (which may not be the size
  /// in memory).
  /// \remark This should be specialized for each subrecord type whose raw type
  ///         is of class type.
  uint16_t size() const {
    return static_cast<uint16_t>(SizeOf(data));
  }

  /// Size of the entire subrecord when written to disk.
  /// This is needed when computing the size of records.
  /// \remark This is a wrapper around size() taking into account header
  ///         information and should *not* be specialized.
  uint32_t entireSize() const {
    return 4u + 2u + size();
  }

  /// Underlying raw subrecord.
  T data{};

  explicit Subrecord(const T &t) : data(t) {}
  explicit Subrecord() = default;
};

/// Write the Subrecord to the stream in the binary representation expected by
/// esp files.
/// \remark This should *not* be specialized for each subrecord type, raw::write
///         should be specialized for `T` if necessary.
template<class T, uint32_t c>
std::ostream &operator<<(std::ostream &os, const Subrecord<T, c> &subrecord) {
  auto size = subrecord.size();
  io::writeBytes(os, recOf<c>());
  io::writeBytes(os, size);
  raw::write(os, subrecord.data, size);

  return os;
}

/// Read a Subrecord stored in its binary representation used in esp files.
/// \remark This should *not* be specialized for each subrecord type, raw::read
///         should be specialized for `T` if necessary.
/// \exception RecordNotFoundError Throw if the subrecord type read does not
///                                match the type of the subrecord.
template<class T, uint32_t c>
std::istream &operator>>(std::istream &is, Subrecord<T, c> &subrecord) {
  std::array<char, 4> type{};
  io::readBytes(is, type);
  if (recOf(type) != c) {
    throw RecordNotFoundError(recOf<c>(), std::string_view(type.data(), 4));
  }

  uint16_t size{};
  io::readBytes(is, size);
  raw::read(is, subrecord.data, size);

  return is;
}

} // namespace record

#endif //OPENOBLIVION_RECORD_SUBRECORD_HPP
