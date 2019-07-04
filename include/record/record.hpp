#ifndef OPENOBL_RECORD_RECORD_HPP
#define OPENOBL_RECORD_RECORD_HPP

#include "io/memstream.hpp"
#include "record/exceptions.hpp"
#include "record/io.hpp"
#include "record/record_header.hpp"
#include "record/rec_of.hpp"

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
  std::size_t size() const {
    return T::size();
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

} // namespace record

namespace io {

template<class T, uint32_t c>
struct BinaryIo<record::Record<T, c>> {
  /// Write the Record to the stream in the binary representation expected by
  /// esp files.
  static void writeBytes(std::ostream &os, const record::Record<T, c> &data) {
    using RecordFlag = record::RecordFlag;
    if ((data.mRecordFlags & RecordFlag::Compressed) != RecordFlag::None) {
      // Write the uncompressed raw record into a buffer.
      const auto uncompressedSize{data.size()};
      std::vector<uint8_t> uncompressedData(uncompressedSize);
      io::memstream mos(uncompressedData.data(), uncompressedSize);
      record::raw::write(mos, static_cast<const T &>(data), uncompressedSize);

      // Compress the raw record into another buffer.
      const auto compressedData{record::compressBytes(uncompressedData)};
      const auto compressedSize{compressedData.size()};

      const auto uncompressedSize32{static_cast<uint32_t>(uncompressedSize)};
      const auto compressedSize32{static_cast<uint32_t>(compressedSize)};

      // Write the record header, where the raw record size is now the size of
      // the compressed data, plus four bytes for the uncompressed size.
      io::writeBytes(os, record::recOf<c>());
      io::writeBytes(os, compressedSize32 + 4u);
      io::writeBytes(os, data.mRecordFlags);
      io::writeBytes(os, data.mFormId);
      io::writeBytes(os, data.mVersionControlInfo);

      // Write the uncompressed size and compressed data.
      io::writeBytes(os, uncompressedSize32);
      os.write(reinterpret_cast<const char *>(compressedData.data()),
               compressedSize);
    } else {
      const auto size{static_cast<uint32_t>(data.size())};
      io::writeBytes(os, record::recOf<c>());
      io::writeBytes(os, size);
      io::writeBytes(os, data.mRecordFlags);
      io::writeBytes(os, data.mFormId);
      io::writeBytes(os, data.mVersionControlInfo);
      record::raw::write(os, static_cast<const T &>(data), size);
    }
  }

  /// Read a Record stored in its binary representation used in esp files.
  /// \exception RecordNotFoundError Thrown if the record type read does not
  ///                                match the type of the record.
  static void readBytes(std::istream &is, record::Record<T, c> &data) {
    std::array<char, 4> type{};
    io::readBytes(is, type);
    if (record::recOf(type) != c) {
      throw record::RecordNotFoundError(record::recOf<c>(),
                                        std::string_view(type.data(), 4));
    }

    // Read the size of the record on disk, which depending on compression may or
    // may not be the actual size of the record.
    uint32_t sizeOnDisk{};
    io::readBytes(is, sizeOnDisk);

    // Read the rest of the record header.
    io::readBytes(is, data.mRecordFlags);
    io::readBytes(is, data.mFormId);
    io::readBytes(is, data.mVersionControlInfo);

    using RecordFlag = record::RecordFlag;
    if ((data.mRecordFlags & RecordFlag::Compressed) != RecordFlag::None) {
      // The size on disk is actually the size of the compressed raw record plus
      // four bytes for the uncompressed size.
      const auto compressedSize{sizeOnDisk - 4u};

      // Read the size of the uncompressed raw record.
      uint32_t uncompressedSize{};
      io::readBytes(is, uncompressedSize);

      // Read the compressed raw record into a buffer.
      std::vector<uint8_t> compressedData{};
      io::readBytes(is, compressedData, compressedSize);

      // Uncompress the raw record into another buffer and interpret it as the
      // raw record.
      const auto uncompressedData{record::uncompressBytes(compressedData,
                                                          uncompressedSize)};
      io::memstream mis(uncompressedData.data(), uncompressedSize);
      record::raw::read(mis, static_cast<T &>(data), uncompressedSize);
    } else {
      // The size on disk is actually the size of the record.
      record::raw::read(is, static_cast<T &>(data), sizeOnDisk);
    }
  }
};

} // namespace io

#endif // OPENOBL_RECORD_RECORD_HPP
