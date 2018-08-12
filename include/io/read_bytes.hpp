#ifndef OPENOBLIVION_READ_BYTES_HPP
#define OPENOBLIVION_READ_BYTES_HPP

#include "io.hpp"
#include <array>
#include <istream>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <typeinfo>
#include <utility>

namespace io {

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

} // namespace io

#endif //OPENOBLIVION_READ_BYTES_HPP
