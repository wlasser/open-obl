#ifndef OPENOBL_RECORD_RECORD_HPP
#define OPENOBL_RECORD_RECORD_HPP

#include "io/memstream.hpp"
#include "record/exceptions.hpp"
#include "record/io.hpp"
#include "record/record_header.hpp"
#include "record/rec_of.hpp"
#include "record/size_of.hpp"

namespace record {

/// Wrapper class for records.
/// \tparam T The raw record type to wrap. Must model the
///           std::DefaultConstructible concept.
/// \tparam c An integer representing the record type. If the type is `"ABCD"`
///           then `c == recOf("ABCD")`.
template<class T, uint32_t c>
class Record : public T {
 public:
  /// The integer representation of the record type.
  constexpr static inline uint32_t RecordType{c};

  /// The underlying raw record type
  using Raw = T;

  RecordFlag mRecordFlags{RecordFlag::None};

  /// BaseId or RefId of this record.
  oo::FormId mFormId{0};

  /// Version control info.
  /// This is bugged in Oblivion, with December coming before January of the
  /// *same* year and not the next one.
  uint32_t mVersionControlInfo{0};

  /// The number of bytes the raw record takes up on disk when saved.
  /// \returns The uncompressed size of the raw record.
  /// \remark This should be specialized for each record type, and it is almost
  ///         certainly *not* `sizeof(T)`; records are saved without any
  ///         padding which may occur in the in-memory representation.
  uint32_t size() const {
    return static_cast<uint32_t>(T::size());
  }

  Record(const T &t,
         RecordFlag flags,
         oo::FormId id,
         uint32_t versionControlInfo) :
      T(t),
      mRecordFlags(flags),
      mFormId(id),
      mVersionControlInfo(versionControlInfo) {}
  Record() : Record(T(), RecordFlag::None, 0, 0) {}
};

/// Write the Record to the stream in the binary representation expected by esp
/// files.
/// \remark This should *not* be specialized for each record type, raw::write
///         should be specialized for `T` instead.
/// \remark The output is 'formatted' in the sense that it is not the object
///         representation of the Record.
template<class T, uint32_t c> std::ostream &
operator<<(std::ostream &os, const Record<T, c> &record) {
  if ((record.mRecordFlags & RecordFlag::Compressed) != RecordFlag::None) {
    // Write the uncompressed raw record into a buffer.
    const unsigned long uncompressedSize{record.size()};
    std::vector<uint8_t> uncompressedData(uncompressedSize);
    io::memstream mos(uncompressedData.data(), uncompressedSize);
    raw::write(mos, static_cast<const T &>(record), uncompressedSize);

    // Compress the raw record into another buffer.
    const auto compressedData{record::compressBytes(uncompressedData)};
    const auto compressedSize{compressedData.size()};

    // Unsigned long may be more than 32 bits.
    const auto uncompressedSize32{static_cast<uint32_t>(uncompressedSize)};
    const auto compressedSize32{static_cast<uint32_t>(compressedSize)};

    // Write the record header, where the raw record size is now the size of
    // the compressed data, plus four bytes for the uncompressed size.
    io::writeBytes(os, recOf<c>());
    io::writeBytes(os, compressedSize32 + 4u);
    io::writeBytes(os, record.mRecordFlags);
    io::writeBytes(os, record.mFormId);
    io::writeBytes(os, record.mVersionControlInfo);

    // Write the uncompressed size and compressed data.
    io::writeBytes(os, uncompressedSize32);
    os.write(reinterpret_cast<const char *>(compressedData.data()),
             compressedSize);
  } else {
    const uint32_t size{record.size()};
    io::writeBytes(os, recOf<c>());
    io::writeBytes(os, size);
    io::writeBytes(os, record.mRecordFlags);
    io::writeBytes(os, record.mFormId);
    io::writeBytes(os, record.mVersionControlInfo);
    raw::write(os, static_cast<const T &>(record), size);
  }

  return os;
}

/// Read a Record stored in its binary representation used in esp files.
/// \remark This should *not* be specialized for each record type, raw::read
///         should be specified for `T` instead.
/// \remark The input is 'formatted' in the sense that it is not the object
///         representation of the Record.
/// \exception RecordNotFoundError Thrown if the record type read does not match
///                                the type of the record.
template<class T, uint32_t c> std::istream &
operator>>(std::istream &is, Record<T, c> &record) {
  std::array<char, 4> type{};
  io::readBytes(is, type);
  if (recOf(type) != c) {
    throw RecordNotFoundError(recOf<c>(), std::string_view(type.data(), 4));
  }

  // Read the size of the record on disk, which depending on compression may or
  // may not be the actual size of the record.
  uint32_t sizeOnDisk{};
  io::readBytes(is, sizeOnDisk);

  // Read the rest of the record header.
  io::readBytes(is, record.mRecordFlags);
  io::readBytes(is, record.mFormId);
  io::readBytes(is, record.mVersionControlInfo);

  if ((record.mRecordFlags & RecordFlag::Compressed) != RecordFlag::None) {
    // The size on disk is actually the size of the compressed raw record plus
    // four bytes for the uncompressed size.
    const unsigned long compressedSize{sizeOnDisk - 4u};

    // Read the size of the uncompressed raw record.
    uint32_t uncompressedSize32{};
    io::readBytes(is, uncompressedSize32);
    unsigned long uncompressedSize{uncompressedSize32};

    // Read the compressed raw record into a buffer.
    std::vector<uint8_t> compressedData{};
    io::readBytes(is, compressedData, compressedSize);

    // Uncompress the raw record into another buffer and interpret it as the
    // raw record.
    const auto uncompressedData{record::uncompressBytes(compressedData,
                                                        uncompressedSize)};
    io::memstream mis(uncompressedData.data(), uncompressedSize);
    raw::read(mis, static_cast<T &>(record), uncompressedSize);
  } else {
    // The size on disk is actually the size of the record.
    raw::read(is, static_cast<T &>(record), sizeOnDisk);
  }

  return is;
}

} // namespace record

#endif // OPENOBL_RECORD_RECORD_HPP
