#ifndef OPENOBLIVION_SUBRECORD_HPP
#define OPENOBLIVION_SUBRECORD_HPP

#include "io/write_bytes.hpp"
#include "io/read_bytes.hpp"
#include "record/io.hpp"
#include "record/size_of.hpp"
#include "record/rec_of.hpp"
#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <utility>

namespace record {

// Wrapper for subrecords
template<class T, uint32_t c>
struct Subrecord {
  static const std::string type;
  // Size of the subrecord data when written to disk (which may not be the size
  // in memory).
  uint16_t size() const {
    return static_cast<uint16_t>(SizeOf(data));
  }
  // Size of the entire subrecord. This is needed when computing the size of
  // records.
  uint32_t entireSize() const {
    return 4 + 2 + size();
  }
  T data{};

  explicit Subrecord(const T &t) : data(t) {}
  explicit Subrecord() = default;
};

template<class T, uint32_t c>
const std::string Subrecord<T, c>::type = recOf<c>();

// Read and write subrecords through a stream. These should not need to be
// specialized, as they make use of the (possibly specialized) raw::read and
// raw::write routines.
template<class T, uint32_t c>
std::ostream &operator<<(std::ostream &os, const Subrecord<T, c> &record) {
  auto size = record.size();
  os.write(record.type.c_str(), 4);
  io::writeBytes(os, size);
  raw::write(os, record.data, size);

  return os;
}

template<class T, uint32_t c>
std::istream &operator>>(std::istream &is, Subrecord<T, c> &record) {
  char type[5]{};
  is.read(type, 4);
  if (record.type != type) throw RecordNotFoundError(record.type, type);

  uint16_t size = 0;
  io::readBytes(is, size);
  raw::read(is, record.data, size);

  return is;
}

// String specialization
template<uint32_t c>
struct Subrecord<std::string, c> {
  static const std::string type;
  uint16_t size() const {
    // A string beginning with a null character has length 1, otherwise the null
    // character is not counted in the length and must be added on. Empty
    // strings have length 0.
    if (data.empty()) return 0;
    else if (data[0] == '\0') return 1;
    else return static_cast<uint16_t>(data.length() + 1);
  }
  uint32_t entireSize() const {
    return 4 + 2 + size();
  }
  std::string data;

  explicit Subrecord(std::string t) : data(std::move(t)) {}
  Subrecord() = default;
};

template<uint32_t c>
const std::string Subrecord<std::string, c>::type = recOf<c>();

} // namespace record

#endif //OPENOBLIVION_SUBRECORD_HPP
