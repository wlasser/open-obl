#ifndef OPENOBLIVION_RECORD_SIZE_OF_HPP
#define OPENOBLIVION_RECORD_SIZE_OF_HPP

#include "bitflag.hpp"
#include "record/tuplifiable.hpp"
#include "record/formid.hpp"
#include <numeric>
#include <array>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace record {

inline std::size_t SizeOf(const oo::BaseId &t) {
  return sizeof(t);
}

inline std::size_t SizeOf(const oo::RefId &t) {
  return sizeof(t);
}

template<class T, typename =
std::enable_if_t<
    std::integral_constant<bool, std::is_integral_v<T>
        || std::is_floating_point_v<T>
        || std::is_enum_v<T>
        || std::is_union_v<T>>::value>>
inline std::size_t SizeOf(const T &t) {
  return sizeof(t);
}

/// \warning A std::string beginning with a '\0' has length 1, otherwise the
///          (trailing) '\0' is not counted in the length and must be added on.
///          Empty std::string have length 0.
inline std::size_t SizeOf(const std::string &t) {
  if (t.empty()) return 0;
  else if (t[0] == '\0') return 1;
  else return static_cast<uint16_t>(t.length() + 1);
}

template<class T> inline std::size_t SizeOf(const std::vector<T> &t) {
  return std::accumulate(t.begin(), t.end(), 0u, [](std::size_t a, const T &b) {
    return a + SizeOf(b);
  });
}

template<class T, std::size_t N>
inline std::size_t SizeOf(const std::array<T, N> &t) {
  if constexpr (N == 0) return 0;
  else return SizeOf(t.front()) * N;
}

template<class T> inline std::size_t SizeOf(const std::optional<T> &t) {
  return t ? SizeOf(t.value()) : 0;
}

template<class T, class S> inline std::size_t SizeOf(const std::pair<T, S> &t) {
  return SizeOf(t.first) + SizeOf(t.second);
}

template<class ...T> inline std::size_t SizeOf(const std::tuple<T...> &t) {
  return std::apply([](const auto &...x) { return (0 + ... + SizeOf(x)); }, t);
}

template<class ...T>
inline std::size_t SizeOf(const std::tuple<const T *...> &t) {
  return std::apply([](const auto &...x) { return (0 + ... + SizeOf(*x)); }, t);
}

template<class ...T> inline std::size_t SizeOf(const Tuplifiable<T...> &t) {
  return SizeOf(t.asTuple());
}

template<class T> inline auto SizeOf(const T &/*data*/) ->
std::enable_if_t<std::is_base_of_v<Bitflag<T::num_bits, T>, T>, std::size_t> {
  return sizeof(typename T::underlying_t);
}

} // namespace record

#endif //OPENOBLIVION_RECORD_SIZE_OF_HPP
