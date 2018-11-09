#ifndef OPENOBLIVION_RECORD_RECORD_HPP
#define OPENOBLIVION_RECORD_RECORD_HPP

#include "formid.hpp"
#include "io/write_bytes.hpp"
#include "io/read_bytes.hpp"
#include "record/exceptions.hpp"
#include "record/record_header.hpp"
#include "record/rec_of.hpp"
#include "record/io.hpp"
#include "record/size_of.hpp"
#include <cstdint>
#include <string>
#include <istream>
#include <ostream>
#include <stdexcept>

namespace record {

/// Wrapper class for records.
/// \tparam T The raw record type to wrap. Must model the
///           std::DefaultConstructible concept.
/// \tparam c An integer representing the record type. If the type is `"ABCD"`
///           then `c == recOf("ABCD")`.
template<class T, uint32_t c>
class Record {
 public:
  /// The integer representation of the record type.
  constexpr static inline uint32_t type{c};

  using Flag = record::RecordFlag;

  Flag flags{Flag::None};

  /// BaseId or RefId of this record.
  FormId id{0};

  /// Version control info.
  /// This is bugged in Oblivion, with December coming before January of the
  /// *same* year and not the next one.
  uint32_t versionControlInfo{0};

  /// Underlying raw record.
  T data{};

  /// The number of bytes `data` takes up on disk when saved.
  /// \remark This should be specialized for each record type, and it is almost
  ///         certainly *not* `sizeof(data)`; records are saved without any
  ///         padding which may occur in the in-memory representation.
  uint32_t size() const {
    return static_cast<uint32_t>(data.size());
  }

  Record(const T &t, Flag flags, FormId id, uint32_t versionControlInfo) :
      flags(flags), id(id), versionControlInfo(versionControlInfo), data(t) {}
  Record() : Record(T(), Flag::None, 0, 0) {}
};

/// Write the Record to the stream in the binary representation expected by esp
/// files.
/// \remark This should *not* be specialized for each record type, raw::write
///         should be specialized for `T` instead.
/// \remark The output is 'formatted' in the sense that it is not the object
///         representation of the Record.
template<class T, uint32_t c>
std::ostream &operator<<(std::ostream &os, const Record<T, c> &record) {
  uint32_t size = record.size();
  io::writeBytes(os, recOf<c>());
  io::writeBytes(os, size);
  io::writeBytes(os, record.flags);
  io::writeBytes(os, record.id);
  io::writeBytes(os, record.versionControlInfo);
  raw::write(os, record.data, size);

  return os;
}

/// Read a Record stored in its binary representation used in esp files.
/// \remark This should *not* be specialized for each record type, raw::read
///         should be specified for `T` instead.
/// \remark The input is 'formatted' in the sense that it is not the object
///         representation of the Record.
/// \exception RecordNotFoundError Thrown if the record type read does not match
///                                the type of the record.
template<class T, uint32_t c>
std::istream &operator>>(std::istream &is, Record<T, c> &record) {
  std::array<char, 4> type{};
  io::readBytes(is, type);
  if (recOf(type) != record.type) {
    throw RecordNotFoundError(recOf<c>(), std::string_view(type.data(), 4));
  }

  uint32_t size{};
  io::readBytes(is, size);
  io::readBytes(is, record.flags);
  io::readBytes(is, record.id);
  io::readBytes(is, record.versionControlInfo);
  raw::read(is, record.data, size);

  return is;
}

} // namespace record

#endif // OPENOBLIVION_RECORD_RECORD_HPP
