#ifndef OPENOBLIVION_RECORD_IO_HPP
#define OPENOBLIVION_RECORD_IO_HPP

#include "io/io.hpp"
#include "io/read_bytes.hpp"
#include "io/write_bytes.hpp"
#include "record/exceptions.hpp"
#include "record/tuplifiable.hpp"
#include <istream>
#include <optional>
#include <ostream>
#include <string>
#include <tuple>
#include <type_traits>

namespace record {

// Peek at the next 4 bytes. If they are a valid (sub)record identified then
// return it, otherwise return an empty string.
std::string peekRecordType(std::istream &);

template<class T>
inline void readOrThrow(std::istream &is,
                        T *data,
                        std::size_t size,
                        const std::string &recordType) {
  if (!io::safeRead(is, data, size)) throw RecordReadError(recordType);
}

inline void peekOrThrow(std::istream &is, const std::string &expected) {
  const std::string peeked = peekRecordType(is);
  if (peeked != expected) throw RecordNotFoundError(expected, peeked);
}

template<class T>
inline void readRecord(std::istream &is, T &t, const std::string &expected) {
  peekOrThrow(is, expected);
  is >> t;
}

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

void skipRecord(std::istream &is);

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
