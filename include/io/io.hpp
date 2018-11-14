#ifndef OPENOBLIVION_IO_IO_HPP
#define OPENOBLIVION_IO_IO_HPP

#include "bitflag.hpp"
#include <boost/format.hpp>
#include <array>
#include <cstddef>
#include <istream>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace io {

// Read raw bytes with a state check, this is used as a shorthand.
template<class T>
inline bool safeRead(std::istream &is, T *data, std::size_t size) {
  is.read(reinterpret_cast<char *>(data), size);
  return is.rdstate() == std::ios::goodbit;
}

std::string decodeIosState(std::ios_base::iostate state);

struct IOReadError : virtual std::runtime_error {
  explicit IOReadError(const std::string &what) : std::runtime_error(what) {}
  IOReadError(const std::string &typeName, std::istream::iostate state) :
      std::runtime_error(boost::str(
          boost::format("Failed to read %s, stream state is %s")
              % typeName % decodeIosState(state))) {}
};

struct byte_direct_ioable_tag {};

template<class T>
struct is_byte_direct_ioable : std::bool_constant<
    std::is_integral_v<T> ||
        std::is_floating_point_v<T> ||
        std::is_enum_v<T> ||
        std::is_union_v<T> ||
        std::is_base_of_v<byte_direct_ioable_tag, T>> {
};

template<class T>
inline constexpr bool is_byte_direct_ioable_v =
    is_byte_direct_ioable<T>::value;

/// Customization point for writeBytes and readBytes.
template<class T>
struct BinaryIo {
  static_assert(is_byte_direct_ioable_v<T>, "Specialize for other types");

  static void writeBytes(std::ostream &os, const T &data) {
    os.write(reinterpret_cast<const char *>(std::addressof(data)),
             sizeof(data));
  }

  static void readBytes(std::istream &is, T &data) {
    is.read(reinterpret_cast<char *>(std::addressof(data)), sizeof(data));
  }
};

template<class T>
void writeBytes(std::ostream &os, const T &data) {
  BinaryIo<T>::writeBytes(os, data);
}

template<class T>
void readBytes(std::istream &is, T &data) {
  BinaryIo<T>::readBytes(is, data);
}

template<class T>
void readBytes(std::istream &is, std::vector<T> &data, std::size_t length) {
  data.assign(length, {});
  if constexpr (is_byte_direct_ioable_v<T>) {
    is.read(reinterpret_cast<char *>(data.data()), length * sizeof(T));
  } else {
    for (auto &elem : data) {
      BinaryIo<T>::readBytes(is, elem);
    }
  }
}

template<>
struct BinaryIo<std::string> {
  static void writeBytes(std::ostream &os, const std::string &data) {
    os.write(data.data(), data.length());
  }

  static void readBytes(std::istream &is, std::string &data) {
    std::getline(is, data, '\0');
  }
};

template<>
struct BinaryIo<std::string_view> {
  static void writeBytes(std::ostream &os, const std::string_view &data) {
    os.write(data.data(), data.length());
  }
};

template<class T, std::size_t N>
struct BinaryIo<std::array<T, N>> {
  static void writeBytes(std::ostream &os, const std::array<T, N> &data) {
    if constexpr (is_byte_direct_ioable_v<T>) {
      os.write(reinterpret_cast<const char *>(data.data()), N * sizeof(T));
    } else {
      for (const auto &elem : data) {
        BinaryIo<T>::writeBytes(os, elem);
      }
    }
  }

  static void readBytes(std::istream &is, std::array<T, N> &data) {
    if constexpr (is_byte_direct_ioable_v<T>) {
      is.read(reinterpret_cast<char *>(data.data()), N * sizeof(T));
    } else {
      for (auto &elem : data) {
        BinaryIo<T>::readBytes(is, elem);
      }
    }
  }
};

template<class T, class S>
struct BinaryIo<std::pair<T, S>> {
  static void writeBytes(std::ostream &os, const std::pair<T, S> &data) {
    BinaryIo<T>::writeBytes(os, std::get<0>(data));
    BinaryIo<S>::writeBytes(os, std::get<1>(data));
  }

  static void readBytes(std::istream &is, std::pair<T, S> &data) {
    BinaryIo<T>::readBytes(is, std::get<0>(data));
    BinaryIo<S>::readBytes(is, std::get<1>(data));
  }
};

template<class T>
struct BinaryIo<std::optional<T>> {
  static_assert(std::is_default_constructible_v<T>,
                "T must be default constructible for readBytes");
  static_assert(std::is_move_constructible_v<T>,
                "T must be move constructible for readBytes");

  static void writeBytes(std::ostream &os, const std::optional<T> &data) {
    if (data) BinaryIo<T>::writeBytes(os, *data);
  }

  static void readBytes(std::istream &is, std::optional<T> &data) {
    T t{};
    BinaryIo<T>::readBytes(is, t);
    data.emplace(std::move(t));
  }
};

template<class ...Ts>
struct BinaryIo<std::tuple<Ts...>> {
  static void writeBytes(std::ostream &os, const std::tuple<Ts...> &data) {
    std::apply([&os](const auto &...x) {
      (BinaryIo<Ts>::writeBytes(os, x), ...);
    }, data);
  }

  static void readBytes(std::istream &is, std::tuple<Ts...> &data) {
    std::apply([&is](auto &...x) {
      (BinaryIo<Ts>::readBytes(is, x), ...);
    }, data);
  }
};

template<std::size_t N, class T>
struct BinaryIo<Bitflag<N, T>> {
  static void writeBytes(std::ostream &os, const Bitflag<N, T> &data) {
    BinaryIo<typename T::underlying_t>::writeBytes(
        os, static_cast<typename T::underlying_t>(data));
  }

  static void readBytes(std::istream &is, Bitflag<N, T> &data) {
    typename T::underlying_t val{};
    BinaryIo<typename T::underlying_t>::readBytes(is, val);
    data = T::make(val);
  }
};

} // namespace io

#endif //OPENOBLIVION_IO_IO_HPP
