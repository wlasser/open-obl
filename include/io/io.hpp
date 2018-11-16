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

/// \defgroup OpenOblivionIo Io Library
/// Provides generic functions for binary io which do The Right Thing on common
/// types.
/// Example usage:
/// ```cpp
/// std::array<char, 3> arr;
/// std::pair<int, float> p;
/// // Read an array of chars, directly followed (i.e. without padding) by an
/// int and float.
/// readBytes(is, arr);
/// readBytes(is, p);
///
/// std::optional<int> opt;
/// if (p.second > 1.0f) opt = 42;
///
/// // Write the data back but insert an integer in the middle if the optional
/// // has a value. If it doesn't, the second writeBytes does nothing.
/// writeBytes(os, arr);
/// writeBytes(os, opt);
/// writeBytes(ps, p);
/// ```
/// The main interface to the library is through the io::readBytes and
/// io::writeBytes function templates, which serialize to and from standard C++
/// streams (namely std::istream and std::ofstream). Objects to serialize and
/// deserialize are taken by reference, which makes it easy to take advantage of
/// template argument deduction and use a uniform syntax for all types.
///
/// Trivially-copyable non-pointer types and arrays, pairs, tuples, and
/// optionals of trivially-copyable non-pointer types (and themselves) are
/// supported by default, with customization of other types supported by
/// specializing the BinaryIo class template.

/// \addtogroup OpenOblivionIo
/// @{
namespace io {

/// Produce a human-readable textual representation of the stream state.
/// \return A pipe (`|`)-separated string of 'goodbit', 'badbit', 'failbit', and
///         'eofbit', with each appearing iff the corresponding bit is set in
///         the state.
/// \remark The ordering of the bits is unspecified.
std::string decodeIosState(std::ios_base::iostate state);

/// Symbolizes that a read operation failed.
/// Can be constructed with the stream state, in which case the exception
/// message contains the result of decodeIosState on that state.
struct IOReadError : virtual std::runtime_error {
  explicit IOReadError(const std::string &what) : std::runtime_error(what) {}
  explicit IOReadError(std::istream::iostate state) :
      std::runtime_error(boost::str(
          boost::format("Failed to read, stream state is %s")
              % decodeIosState(state))) {}
};

/// \name ByteDirectIoable
/// The *ByteDirectIoable* concept is modelled by those types which can be
/// (de)serialized through a simple std::istream::read and std::ostream::write
/// applied to their [object representation]
/// (https://en.cppreference.com/w/cpp/language/object#Object_representation_and_value_representation).
/// All non-pointer *TriviallyCopyable* types are *ByteDirectIoable*.
/// User-defined types may opt-in by deriving from the byte_direct_ioable_tag.
///@{

/// User-types which wish to be *ByteDirectIoable* should inherit from this.
struct byte_direct_ioable_tag {};

/// Cbecks whether T is *ByteDirectIoable*.
/// Provides the member constant `value`  which is equal to `true` if T is
/// *ByteDirectIoable*. Otherwise, `value` is equal to `false`.
/// \tparam The type to check
template<class T>
struct is_byte_direct_ioable : std::bool_constant<
    (!std::is_pointer_v<T> && std::is_trivially_copyable_v<T>) ||
        std::is_base_of_v<byte_direct_ioable_tag, T>> {
};

/// Helper variable template for is_byte_direct_ioable.
template<class T>
inline constexpr bool is_byte_direct_ioable_v =
    is_byte_direct_ioable<T>::value;

///@}

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

/// Seralize data to the a stream.
/// \param os The stream to write to
/// \param data The object to write
template<class T>
void writeBytes(std::ostream &os, const T &data) {
  BinaryIo<T>::writeBytes(os, data);
}

/// Deserialize data from a stream.
/// \param is The stream to read from
/// \param data The object to read into
template<class T>
void readBytes(std::istream &is, T &data) {
  BinaryIo<T>::readBytes(is, data);
  if (!is) throw IOReadError(is.rdstate());
}

/// Deserialize data from a stream. It is expected that data has been default-
/// constructed, it is not required to be the correct length.
/// \param is The stream to read from
/// \param data The object to read into
/// \param length The number of **elements** to read.
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

/// Customization for std::string.
/// \warning Expects a null-terminator when reading, does not output one when
///          writing!
/// \todo Make this output a null-terminator.
template<>
struct BinaryIo<std::string> {
  static void writeBytes(std::ostream &os, const std::string &data) {
    os.write(data.data(), data.length());
  }

  static void readBytes(std::istream &is, std::string &data) {
    std::getline(is, data, '\0');
  }
};

/// Customization for std::string_view.
/// Only writeBytes is supported as std::string_view is read-only.
/// \remark Does not output a null-terminator.
template<>
struct BinaryIo<std::string_view> {
  static void writeBytes(std::ostream &os, const std::string_view &data) {
    os.write(data.data(), data.length());
  }
};

/// Customization for std::array.
/// If `T` is *ByteDirectIoable* then all elements of the array are
/// (de)serialized simultaneously.
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

/// Customization for std::pair.
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

/// Customization for std::optional.
/// \tparam T Must be *DefaultConstructible* and *MoveConstructible*.
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

/// Customization for std::tuple.
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

/// Customization for Bitflag.
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
/// @}

#endif //OPENOBLIVION_IO_IO_HPP
