#ifndef OPENOBL_RECORD_SUBRECORD_HPP
#define OPENOBL_RECORD_SUBRECORD_HPP

#include "record/exceptions.hpp"
#include "record/io.hpp"
#include "record/size_of.hpp"
#include "record/rec_of.hpp"
#include <array>
#include <cstdint>
#include <string_view>

namespace record {

namespace raw {

/// This should be specialized for each raw subrecord type for which `SizeOf`
/// would not return the correct result.
template<class T>
struct SubrecordSize {
  std::size_t operator()(const T &data) const { return SizeOf(data); }
};

} // namespace raw

/// Wrapper class for subrecords.
/// \tparam T The raw subrecord type to wrap. Must model the
///           std::DefaultConstructible concept.
/// \tparam c An integer representing the subrecord type. If the subrecord type
///           is "ABCD" then `c == recOf("ABCD")`.
template<class T, uint32_t c>
struct Subrecord {
  /// The integer representation of the subrecord type.
  constexpr static inline uint32_t RecordType{c};

  /// Size of the raw subrecord data when written to disk (which may not be the
  /// size in memory).
  /// \remark This is a wrapper around SubrecordSize and should *not* be
  ///         specialized.
  std::size_t size() const {
    return raw::SubrecordSize<T>()(data);
  }

  /// Size of the entire subrecord when written to disk.
  /// This is needed when computing the size of records.
  /// \remark This is a wrapper around size() taking into account header
  ///         information and should *not* be specialized.
  std::size_t entireSize() const {
    return 4u + 2u + size();
  }

  /// Underlying raw subrecord.
  T data{};

  explicit Subrecord(const T &t) : data(t) {}
  explicit Subrecord() = default;
};

template<class T, uint32_t c>
std::size_t SizeOf(const Subrecord<T, c> &t) {
  return t.entireSize();
}

} // namespace record

namespace io {

template<class T, uint32_t c>
struct BinaryIo<record::Subrecord<T, c>> {
  /// Write the Subrecord to the stream in the binary representation expected by
  /// esp files.
  static void writeBytes(std::ostream &os,
                         const record::Subrecord<T, c> &data) {
    const auto size{data.size()};
    const auto size16{static_cast<uint16_t>(size)};
    io::writeBytes(os, record::recOf<c>());
    io::writeBytes(os, size16);
    record::raw::write(os, data.data, size);
  }

  /// Read a Subrecord stored in its binary representation used in esp files.
  /// \exception RecordNotFoundError Throw if the subrecord type read does not
  ///                                match the type of the subrecord.
  static void readBytes(std::istream &is, record::Subrecord<T, c> &data) {
    std::array<char, 4> type{};
    io::readBytes(is, type);
    if (record::recOf(type) != c) {
      throw record::RecordNotFoundError(record::recOf<c>(),
                                        std::string_view(type.data(), 4));
    }

    uint16_t size{};
    io::readBytes(is, size);
    record::raw::read(is, data.data, size);
  }
};

} // namespace io

#endif //OPENOBL_RECORD_SUBRECORD_HPP
