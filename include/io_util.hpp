#ifndef IO_UTIL_HPP
#define IO_UTIL_HPP

#include <string>
#include <istream>
#include <memory>
#include <optional>
#include <array>
#include <typeinfo>

inline std::string decodeIosState(std::ios_base::iostate state) {
  switch (state) {
    case std::ios::goodbit: return "goodbit";
    case std::ios::badbit: return "badbit";
    case std::ios::failbit: return "failbit";
    case std::ios::eofbit: return "eofbit";
    default: return "unknown";
  }
}

class IOReadError : public std::runtime_error {
 public:
  explicit IOReadError(const std::string &what) : std::runtime_error(what) {}
  IOReadError(const std::string &typeName, std::istream::iostate state) :
      std::runtime_error(std::string("Failed to read ")
                             .append(typeName)
                             .append(", stream state is ")
                             .append(decodeIosState(state))) {}
};

// Read a null-terminated string prefixed with a single byte for the length.
std::string readBzString(std::istream &);

// Read a string prefixed with a single byte for the length, without a
// null-terminator.
std::string readBString(std::istream &);

// Read raw bytes with a state check, this is used as a shorthand.
template<class T>
inline bool safeRead(std::istream &is, T *data, std::size_t size) {
  is.read(reinterpret_cast<char *>(data), size);
  return is.rdstate() == std::ios::goodbit;
}

template<class T>
void readBytes(std::istream &is, T &data) {
  is.read(reinterpret_cast<char *>(std::addressof(data)), sizeof(data));
  if (is.rdstate() != std::ios::goodbit) {
    throw IOReadError(typeid(T).name(), is.rdstate());
  }
}

template<>
void readBytes(std::istream &is, std::string &data);

template<class T, std::size_t N>
void readBytes(std::istream &is, std::array<T, N> &data) {
  is.read(reinterpret_cast<char *>(data.data()), N * sizeof(T));
  if (is.rdstate() != std::ios::goodbit) {
    throw IOReadError(typeid(T).name(), is.rdstate());
  }
}

template<class T>
void readBytes(std::istream &is, std::optional<T> &data) {
  T val{};
  readBytes(is, val);
  data.emplace(val);
}

template<class T, class S>
void readBytes(std::istream &is, std::pair<T, S> &data) {
  readBytes(is, data.first);
  readBytes(is, data.second);
}

template<class ... T>
void readBytes(std::istream &is, std::tuple<T...> &data) {
  std::apply([&is](auto &...x) { (readBytes(is, x), ...); }, data);
}

// Write raw bytes to a stream.
template<class T>
inline void writeBytes(std::ostream &os, const T &data) {
  os.write(reinterpret_cast<const char *>(std::addressof(data)),
           sizeof(data));
}

// TODO: write null terminator?
template<>
inline void writeBytes(std::ostream &os, const std::string &data) {
  os.write(data.c_str(), data.length());
}

template<class T, std::size_t N>
inline void writeBytes(std::ostream &os, const std::array<T, N> &data) {
  os.write(reinterpret_cast<const char *>(data.data()), N * sizeof(T));
}

template<class T, class S>
inline void writeBytes(std::ostream &os, const std::pair<T, S> &data) {
  writeBytes(os, data.first);
  writeBytes(os, data.second);
}

template<class T>
inline void writeBytes(std::ostream &os, const std::optional<T> &data) {
  if (data) writeBytes(os, data.value());
}

template<class ... T>
void writeBytes(std::ostream &os, const std::tuple<T...> &data) {
  std::apply([&os](const auto &...x) { (writeBytes(os, x), ...); }, data);
}

// Peek at the next 4 bytes. If they are a valid (sub)record identified then
// return it, otherwise return an empty string.
std::string peekRecordType(std::istream &);

// Stream wrapping binary data. Does not take ownership.
class memstream : public std::istream {
 private:
  class membuf : public std::basic_streambuf<char> {
   public:
    membuf(const uint8_t *p, std::size_t l) {
      setg((char *) (p), (char *) (p), (char *) (p) + l);
    }
  };

  membuf buffer;

 public:
  memstream(const uint8_t *p, std::size_t l) :
      std::istream(&buffer),
      buffer(p, l) {
    rdbuf(&buffer);
  }
};

#endif // IO_UTIL_HPP
