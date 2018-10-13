#ifndef OPENOBLIVION_IO_READ_BYTES_HPP
#define OPENOBLIVION_IO_READ_BYTES_HPP

#include "io/io.hpp"
#include <array>
#include <cstddef>
#include <istream>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <typeinfo>
#include <utility>
#include <vector>

namespace io {

template<class T>
struct is_byte_direct_readable : std::bool_constant<
    std::is_integral_v<T> ||
        std::is_floating_point_v<T> ||
        std::is_enum_v<T> ||
        std::is_aggregate_v<T>> {
};

template<class T>
inline constexpr bool is_byte_direct_readable_v =
    is_byte_direct_readable<T>::value;

template<class T>
auto readBytes(std::istream &is, T &data)
-> typename std::enable_if_t<is_byte_direct_readable_v<T>, void> {
  is.read(reinterpret_cast<char *>(std::addressof(data)), sizeof(data));
  if (is.rdstate() != std::ios::goodbit) {
    throw IOReadError(typeid(T).name(), is.rdstate());
  }
}

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

template<class T>
void readBytes(std::istream &is, std::vector<T> &data, std::size_t length) {
  data.reserve(length);
  for (std::size_t i = 0; i < length; ++i) {
    data.template emplace_back<T>({});
    readBytes(is, data.back());
  }
}

} // namespace io

#endif //OPENOBLIVION_IO_READ_BYTES_HPP
