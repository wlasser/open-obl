#ifndef OPENOBL_RECORD_TUPLIFIABLE_HPP
#define OPENOBL_RECORD_TUPLIFIABLE_HPP

#include "io/io.hpp"
#include <tuple>

namespace record {

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

} // namespace record

namespace io {

template<class T>
struct BinaryIo<T, std::enable_if_t<
    std::is_base_of_v<record::TuplifiableMarker, T>>> {
 private:
  template<class ...Ts> static void
  writeBytesImpl(std::ostream &os, const record::Tuplifiable<Ts...> &t) {
    std::apply([&os](auto ...x) { (BinaryIo<Ts>::writeBytes(os, *x), ...); },
               t.asTuple());
  }

  template<class ...Ts> static void
  readBytesImpl(std::istream &is, record::Tuplifiable<Ts...> &t) {
    std::apply([&is](auto ...x) { (BinaryIo<Ts>::readBytes(is, *x), ...); },
               t.asTuple());
  }

 public:
  static void writeBytes(std::ostream &os, const T &t) {
    return BinaryIo::writeBytesImpl(os, t);
  }

  static void readBytes(std::istream &is, T &t) {
    return BinaryIo::readBytesImpl(is, t);
  }
};

} // namespace io

#endif //OPENOBL_RECORD_TUPLIFIABLE_HPP
