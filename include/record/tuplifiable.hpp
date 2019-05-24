#ifndef OPENOBL_RECORD_TUPLIFIABLE_HPP
#define OPENOBL_RECORD_TUPLIFIABLE_HPP

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

#endif //OPENOBL_RECORD_TUPLIFIABLE_HPP
