#ifndef RECORD_HPP
#define RECORD_HPP

#include <string>
#include <istream>
#include <ostream>
#include <array>
#include <stdexcept>
#include <vector>
#include <optional>
#include <tuple>
#include "actor_value.hpp"
#include "formid.hpp"
#include "magic_effects.hpp"
#include "io/io.hpp"
#include "io/write_bytes.hpp"
#include "io/read_bytes.hpp"

namespace record {

class RecordReadError : public std::runtime_error {
 public:
  explicit RecordReadError(const std::string &recordType) :
      std::runtime_error(std::string("Failed to read ") + recordType
                             + std::string("record")) {}
};

class RecordNotFoundError : public std::runtime_error {
 public:
  RecordNotFoundError(const std::string &expected, const std::string &actual) :
      std::runtime_error(std::string("Expected ") + expected + ", found "
                             + std::string(actual)) {}
};

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

// Convert a 4-character string literal to a little-endian integer
inline constexpr uint32_t recOf(const char *r, std::size_t size) {
  return size != 4 ? 0u : (static_cast<uint8_t>(r[0])
      + (static_cast<uint8_t>(r[1]) << 8u)
      + (static_cast<uint8_t>(r[2]) << 16u)
      + (static_cast<uint8_t>(r[3]) << 24u));
}
inline constexpr uint32_t recOf(const std::array<char, 4> &r) {
  return recOf(r.data(), 4);
}
inline constexpr uint32_t operator "" _rec(const char *r, std::size_t size) {
  return recOf(r, size);
}

// Convert back from a integer to a 4-character string literal
template<uint32_t t>
const char *recOf() {
  static const char r[] = {
      static_cast<const char>(t & 0xffu),
      static_cast<const char>((t & (0xffu << 8u)) >> 8u),
      static_cast<const char>((t & (0xffu << 16u)) >> 16u),
      static_cast<const char>((t & (0x0ffu << 24u)) >> 24u),
      '\0'
  };
  return r;
}

// TODO: Support nested tuplifiables
struct TuplifiableMarker {};
template<class ...T>
struct Tuplifiable : TuplifiableMarker {
  using tuple_t = std::tuple<T *...>;
  using const_tuple_t = std::tuple<T const *...>;
  virtual tuple_t asTuple() = 0;
  virtual const_tuple_t asTuple() const = 0;
  virtual ~Tuplifiable() = default;
};

#define MAKE_AS_TUPLE(...) \
  tuple_t asTuple() override { return {__VA_ARGS__}; } \
  const_tuple_t asTuple() const override { return {__VA_ARGS__}; }

template<class T, typename =
std::enable_if_t<
    std::integral_constant<bool, std::is_integral_v<T>
        || std::is_floating_point_v<T>
        || std::is_enum_v<T>
        || std::is_union_v<T>>::value>>
inline std::size_t SizeOf(const T &t) {
  return sizeof(t);
}

inline std::size_t SizeOf(const std::string &t) {
  return t.length();
}

template<class T>
inline std::size_t SizeOf(const std::vector<T> &t) {
  return t.empty() ? 0 : (t.size() * SizeOf(t.front()));
}

template<class T, std::size_t N>
inline std::size_t SizeOf(const std::array<T, N> &t) {
  if constexpr (N == 0) return 0;
  else return SizeOf(t.front()) * N;
}

template<class T>
inline std::size_t SizeOf(const std::optional<T> &t) {
  return t ? SizeOf(t.value()) : 0;
}

template<class T, class S>
inline std::size_t SizeOf(const std::pair<T, S> &t) {
  return SizeOf(t.first) + SizeOf(t.second);
}

template<class ...T>
inline std::size_t SizeOf(const std::tuple<T...> &t) {
  return std::apply([](const auto &...x) { return (0 + ... +SizeOf(x)); }, t);
}

template<class ...T>
inline std::size_t SizeOf(const std::tuple<const T *...> &t) {
  return std::apply([](const auto &...x) { return (0 + ... +SizeOf(*x)); }, t);
}

template<class ...T>
inline std::size_t SizeOf(const Tuplifiable<T...> &t) {
  return SizeOf(t.asTuple());
}

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

// Wrapper for records
template<class T, uint32_t c>
class Record {
 public:
  const std::string type = recOf<c>();
  uint32_t size() const {
    return static_cast<uint32_t>(data.size());
  }
  enum class Flag : uint32_t {
    None = 0,
    ESM = 0x00000001,
    Deleted = 0x00000020,
    CastsShadows = 0x00000200,
    QuestOrPersistent = 0x00000400,
    InitiallyDisabled = 0x00000800,
    Ignored = 0x00001000,
    VisibleWhenDistant = 0x00008000,
    OffLimits = 0x00020000,
    Compressed = 0x00040000,
    CantWait = 0x00080000
  };
  Flag flags = Flag::None;
  FormID id = 0;
  // Version control info. This is bugged in the original implementation, with
  // Dec coming before Jan of the same year and not the next one. We ignore it.
  uint32_t versionControlInfo = 0;
  T data;

  Record(const T &t, Flag flags, FormID id, uint32_t versionControlInfo) :
      flags(flags), id(id), versionControlInfo(versionControlInfo), data(t) {}
  Record() : Record(T(), Flag::None, 0, 0) {}
};

// Read and write records through a stream. Like the subrecord stream IO
// functions, these should not need to be specialized; all the
// specialization is done in raw::read and raw::write.
template<class T, uint32_t c>
std::ostream &operator<<(std::ostream &os, const Record<T, c> &record) {
  uint32_t size = record.size();
  os.write(record.type.data(), 4);
  io::writeBytes(os, size);
  io::writeBytes(os, record.flags);
  io::writeBytes(os, record.id);
  io::writeBytes(os, record.versionControlInfo);
  raw::write(os, record.data, size);

  return os;
}

template<class T, uint32_t c>
std::istream &operator>>(std::istream &is, Record<T, c> &record) {
  char type[5]{};
  is.read(type, 4);
  if (record.type != type) throw RecordNotFoundError(record.type, type);

  uint32_t size;
  io::readBytes(is, size);
  io::readBytes(is, record.flags);
  io::readBytes(is, record.id);
  io::readBytes(is, record.versionControlInfo);
  raw::read(is, record.data, size);

  return is;
}

} // namespace record

#endif // RECORD_HPP
