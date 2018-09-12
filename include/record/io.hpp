#ifndef OPENOBLIVION_RECORD_IO_HPP
#define OPENOBLIVION_RECORD_IO_HPP

#include "io/io.hpp"
#include "io/read_bytes.hpp"
#include "io/write_bytes.hpp"
#include "record/exceptions.hpp"
#include "record/tuplifiable.hpp"
#include "record/record_header.hpp"
#include <istream>
#include <optional>
#include <ostream>
#include <string>
#include <tuple>
#include <type_traits>

namespace record {

// Peek at the next 4 bytes and return them as a string
// TODO: Is it worth the string construction? Could return a uint32_t directly.
std::string peekRecordType(std::istream &);

// Read the type `T` of size `size` into `data`, throwing if an io error occurs.
// Intended to be called by a function reading a (sub)record, whereupon
// `recordType` should be the type of the sub(record) being read.
template<class T>
inline void readOrThrow(std::istream &is,
                        T *data,
                        std::size_t size,
                        const std::string &recordType) {
  if (!io::safeRead(is, data, size)) throw RecordReadError(recordType);
}

// Call `peekRecordType` and throw if the returned type is not `expected`
inline void peekOrThrow(std::istream &is, const std::string &expected) {
  const std::string peeked = peekRecordType(is);
  if (peeked != expected) throw RecordNotFoundError(expected, peeked);
}

// Read the (sub)record of the `expected` type into `t` and throw if the actual
// type is not the `expected` one.
// This function is not really needed. `expected` is already known by `t.type`,
// and for `T` a Record or Subrecord (as it should be) >> is specified to throw
// if the type is not the expected one, so the peek is not even necessary.
// The only advantage seems to be that we get the strong exception guarantee
// (provided string construction doesn't throw) by using `peekOrThrow`, which
// >> by itself does not provide. This is a flaw of >> however, I think.
// TODO: This is a useless wrapper around `>>`, remove it.
template<class T>
inline void readRecord(std::istream &is, T &t, const std::string &expected) {
  peekOrThrow(is, expected);
  is >> t;
}

// If the next record is `expected` then read it, otherwise null the optional.
template<class T>
inline void readRecord(std::istream &is,
                       std::optional<T> &t,
                       const std::string &expected) {
  if (peekRecordType(is) == expected) {
    T r{};
    is >> r;
    t.emplace(r);
  } else {
    t = std::nullopt;
  }
}

// Read the next record of type `T`, throwing if the type is unexpected or
// there's an io error.
template<class T>
T readRecord(std::istream &is) {
  T rec;
  is >> rec;
  if (!is.good()) throw io::IOReadError(rec.type, is.rdstate());
  return rec;
}

// Read the header of the next record and place `is` just before the body.
// Does not check that `is` is pointing to a record.
RecordHeader readRecordHeader(std::istream &is);
// Skip the next record, returning its header.
// Does not check that `is` is pointing to a record.
RecordHeader skipRecord(std::istream &is);
// Skip the next group.
// Does not check that `is` is pointing to a group.
void skipGroup(std::istream &is);

namespace raw {

// These read and write raw (sub)records as bytes to and from streams. This will
// only match the desired output for integral types, and should be specialized
// otherwise.
template<class T, typename = std::enable_if_t<!std::is_base_of<TuplifiableMarker,
                                                               T>::value>>
std::ostream &write(std::ostream &os, const T &t, std::size_t /*size*/) {
  io::writeBytes(os, t);
  return os;
}

template<class T, typename = std::enable_if_t<!std::is_base_of<TuplifiableMarker,
                                                               T>::value>>
std::istream &read(std::istream &is, T &t, std::size_t size) {
  if (size != 0) io::readBytes(is, t);
  return is;
}

template<class ...T>
std::ostream &write(std::ostream &os,
                    const Tuplifiable<T...> &t,
                    std::size_t /*size*/) {
  std::apply([&os](const auto &...x) { (io::writeBytes(os, *x), ...); },
             t.asTuple());
  return os;
}

template<class ...T>
std::istream &read(std::istream &is, Tuplifiable<T...> &t, std::size_t size) {
  std::apply([&is](auto ...x) { (io::readBytes(is, *x), ...); }, t.asTuple());
  return is;
}

} // namespace raw

} // namespace record

#endif //OPENOBLIVION_RECORD_IO_HPP
