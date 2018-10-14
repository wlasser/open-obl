#ifndef OPENOBLIVION_IO_WRITE_BYTES_HPP
#define OPENOBLIVION_IO_WRITE_BYTES_HPP

#include "bitflag.hpp"
#include "io/io.hpp"
#include <array>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

namespace io {

// Write raw bytes to a stream.
template<class T>
inline auto writeBytes(std::ostream &os, const T &data) ->
    std::enable_if_t<is_byte_direct_ioable_v < T>,
void> {
os.write(reinterpret_cast
<const char *>(std::addressof(data)),
sizeof(data));
}

// TODO: write null terminator?
inline void writeBytes(std::ostream &os, const std::string &data) {
  os.write(data.c_str(), data.length());
}

template<class T, std::size_t N>
inline void writeBytes(std::ostream &os, const std::array<T, N> &data) {
  os.write(reinterpret_cast<const char *>(data.data()), N * sizeof(T));
}

template<class T, class S>
inline void writeBytes(std::ostream &os, const std::pair<T, S> &data) {
  writeBytes(os, data.first);
  writeBytes(os, data.second);
}

template<class T>
inline void writeBytes(std::ostream &os, const std::optional<T> &data) {
  if (data) writeBytes(os, data.value());
}

template<class ... T>
void writeBytes(std::ostream &os, const std::tuple<T...> &data) {
  std::apply([&os](const auto &...x) { (writeBytes(os, x), ...); }, data);
}

template<class T>
auto writeBytes(std::ostream &os, T &data) -> std::enable_if_t<
    std::is_base_of_v<
        Bitflag<std::remove_cv_t<T>::num_bits, std::remove_cv_t<T>>,
        std::remove_cv_t<T>>, void> {
  writeBytes(os, static_cast<typename T::underlying_t>(data));
}

} // namespace io

#endif //OPENOBLIVION_IO_WRITE_BYTES_HPP
