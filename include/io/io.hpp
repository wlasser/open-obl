#ifndef OPENOBLIVION_IO_IO_HPP
#define OPENOBLIVION_IO_IO_HPP

#include <boost/format.hpp>
#include <cstddef>
#include <istream>
#include <stdexcept>
#include <string>

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

} // namespace io

#endif //OPENOBLIVION_IO_IO_HPP
