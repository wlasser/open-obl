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
