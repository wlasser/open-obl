#ifndef OPENOBL_RECORD_IO_HPP
#define OPENOBL_RECORD_IO_HPP

#include "io/io.hpp"
#include "record/record_header.hpp"

namespace record {

/// Return the integer representation of the next 4 bytes, which is hopefully a
/// record type.
uint32_t peekRecordType(std::istream &is) noexcept;

/// Wrapper around io::readBytes
/// \tparam T Should be a Subrecord or Record
/// \todo Fix the SFINAE on this to actually only match Subrecord or Record
template<class T, uint32_t recType = T::RecordType>
void readRecord(std::istream &is, T &t) {
  io::readBytes(is, t);
}

/// If the next record type is c then read into t, otherwise reset t.
/// \tparam T Should be a Subrecord or Record
/// \todo Fix the SFINAE on this to actually only match Subrecord or Record
template<class T, uint32_t recType = T::RecordType>
void readRecord(std::istream &is, std::optional<T> &t) {
  if (record::peekRecordType(is) == recType) {
    io::readBytes(is, t);
  } else {
    t.reset();
  }
}

/// Wrapper around io::readBytes
/// \overload readRecord(std::istream &, T &)
template<class T, uint32_t c = T::RecordType>
T readRecord(std::istream &is) {
  T rec{};
  io::readBytes(is, rec);
  return rec;
}

/// Read sequential records of type c.
/// \tparam T should be a Subrecord or Record
/// \todo Fix the SFINAE on this to actually only match Subrecord or Record
template<class T, uint32_t recType = T::RecordType>
void readRecord(std::istream &is, std::vector<T> &t) {
  while (record::peekRecordType(is) == recType) {
    t.emplace_back(readRecord<T>(is));
  }
}

/// Wrapper around io::writeBytes
/// \tparam T Should be a Subrecord or Record.
/// \todo Fix the SFINAE on this to actually only match Subrecord or Record
template<class T, uint32_t recType = T::RecordType>
void writeRecord(std::ostream &os, const T &t) {
  io::writeBytes(os, t);
}

/// If t has contents then write it with writeRecord(), otherwise no dothing.
/// \tparam T Should be a Subrecord or Record.
/// \todo Fix the SFINAE on this to actually only match Subrecord or Record
template<class T, uint32_t recType = T::RecordType>
void writeRecord(std::ostream &os, const std::optional<T> &t) {
  io::writeBytes(os, t);
}

/// If t is nonempty then write its contents sequentially with writeRecord(),
/// otherwise do nothing.
/// \tparam T Should be a Subrecord or Record.
/// \todo Fix the SFINAE on this to actually match Subrecord or Record.
template<class T, uint32_t = T::RecordType>
void writeRecord(std::ostream &os, const std::vector<T> &t) {
  for (const auto &r : t) writeRecord(os, r);
}

/// Read the header of the next record and place `is` just before the body.
/// Does not check that `is` is pointing to a record.
RecordHeader readRecordHeader(std::istream &is);

/// Skip the next record, returning its header.
/// Does not check that `is` is pointing to a record.
RecordHeader skipRecord(std::istream &is);

/// Skip the next group.
/// Does not check that `is` is pointing to a group.
void skipGroup(std::istream &is);

/// Compress a uncompressed array `uncomp` of bytes.
std::vector<uint8_t> compressBytes(const std::vector<uint8_t> &uncomp);

/// Uncompress a compressed array `comp` of bytes, given an upper bound
/// `uncompSize` for the size of the uncompressed data.
std::vector<uint8_t> uncompressBytes(const std::vector<uint8_t> &comp,
                                     std::size_t uncompSize);

namespace raw {

/// \remark Specialize this for each raw type of non-class type that is not
///         Tuplifiable.
template<class T> struct SizedBinaryIo {
  /// Output the raw Record or raw Subrecord T in its esp binary representation.
  static void writeBytes(std::ostream &os, const T &t, std::size_t /*size*/) {
    io::writeBytes(os, t);
  }

  /// Read a raw Record or raw Subrecord T in its esp binary representation.
  static void readBytes(std::istream &is, T &t, std::size_t size) {
    if (size != 0u) io::readBytes(is, t);
  }
};

template<class T>
void write(std::ostream &os, const T &t, std::size_t size) {
  SizedBinaryIo<T>::writeBytes(os, t, size);
}

template<class T>
void read(std::istream &is, T &t, std::size_t size) {
  SizedBinaryIo<T>::readBytes(is, t, size);
}

} // namespace raw

} // namespace record

#endif //OPENOBL_RECORD_IO_HPP
