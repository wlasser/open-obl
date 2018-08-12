#ifndef OPENOBLIVION_SIZE_OF_HPP
#define OPENOBLIVION_SIZE_OF_HPP

#include "record/tuplifiable.hpp"
#include <array>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace record {

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

} // namespace record

#endif //OPENOBLIVION_SIZE_OF_HPP
