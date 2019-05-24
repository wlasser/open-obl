#ifndef OPENOBL_RECORD_IO_HPP
#define OPENOBL_RECORD_IO_HPP

#include "io/io.hpp"
#include "record/tuplifiable.hpp"
#include "record/rec_of.hpp"
#include "record/record_header.hpp"
#include <type_traits>

namespace record {

/// Return the integer representation of the next 4 bytes, which is hopefully a
/// record type.
uint32_t peekRecordType(std::istream &is) noexcept;

/// Wrapper around operator>>(std::istream &, T&)
/// \tparam T Should be a Subrecord or Record
/// \todo Fix the SFINAE on this to actually only match Subrecord or Record
template<class T, uint32_t recType = T::RecordType>
void readRecord(std::istream &is, T &t) {
  is >> t;
}

/// If the next record type is c then read into t, otherwise reset t.
/// \tparam T Should be a Subrecord or Record
/// \todo Fix the SFINAE on this to actually only match Subrecord or Record
template<class T, uint32_t recType = T::RecordType>
void readRecord(std::istream &is, std::optional<T> &t) {
  if (peekRecordType(is) == recType) {
    T r{};
    is >> r;
    t.emplace(std::move(r));
  } else {
    t.reset();
  }
}

/// Wrapper around operator>>(std::istream &, T&)
/// \overload readRecord(std::istream &, T &)
template<class T, uint32_t c = T::RecordType>
T readRecord(std::istream &is) {
  T rec{};
  is >> rec;
  return rec;
}

/// Wrapper around operator<<(std::ostream &, const T&).
/// \tparam T Should be a Subrecord or Record.
/// \todo Fix the SFINAE on this to actually only match Subrecord or Record
template<class T, uint32_t recType = T::RecordType>
void writeRecord(std::ostream &os, const T &t) {
  os << t;
}

/// If t has contents then write it with writeRecord(), otherwise no dothing.
/// \tparam T Should be a Subrecord or Record.
/// \todo Fix the SFINAE on this to actually only match Subrecord or Record
template<class T, uint32_t recType = T::RecordType>
void writeRecord(std::ostream &os, const std::optional<T> &t) {
  if (t) os << *t;
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

/// Output the raw Record or raw Subrecord T in its esp binary representation.
/// \remark Specialize this for each raw type of non-class type that is not
///         Tuplifiable or a Bitflag.
template<class T, typename = std::enable_if_t<
    !std::is_base_of<TuplifiableMarker, T>::value
        && !std::is_base_of<BitflagMarker, T>::value>>
std::ostream &write(std::ostream &os, const T &t, std::size_t /*size*/) {
  io::writeBytes(os, t);
  return os;
}

/// Read a raw Record or raw Subrecord T in its esp binary representation.
/// \remark Specialize this for each raw type of non-class type that is not
///         Tuplifiable.
template<class T, typename = std::enable_if_t<
    !std::is_base_of<TuplifiableMarker, T>::value
        && !std::is_base_of<BitflagMarker, T>::value>>
std::istream &read(std::istream &is, T &t, std::size_t size) {
  if (size != 0) io::readBytes(is, t);
  return is;
}

template<std::size_t N, class T>
std::ostream &write(std::ostream &os, const Bitflag<N, T> &t, std::size_t) {
  io::writeBytes(os, t);
  return os;
}

template<std::size_t N, class T>
std::istream &read(std::istream &is, Bitflag<N, T> &t, std::size_t) {
  io::readBytes(is, t);
  return is;
}

/// \overload raw::write(std::ostream &, const T &, std::size_t)
template<class ...T>
std::ostream &write(std::ostream &os,
                    const Tuplifiable<T...> &t,
                    std::size_t /*size*/) {
  std::apply([&os](const auto &...x) { (io::writeBytes(os, *x), ...); },
             t.asTuple());
  return os;
}

/// \overload raw::read(std::istream &, T &, std::size_t)
template<class ...T>
std::istream &read(std::istream &is,
                   Tuplifiable<T...> &t,
                   std::size_t /*size*/) {
  std::apply([&is](auto ...x) { (io::readBytes(is, *x), ...); }, t.asTuple());
  return is;
}

} // namespace raw

} // namespace record

#endif //OPENOBL_RECORD_IO_HPP
