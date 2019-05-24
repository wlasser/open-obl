#ifndef OPENOBL_IO_STRING_HPP
#define OPENOBL_IO_STRING_HPP

#include <string>
#include <istream>

namespace io {

/// Read a null-terminated string prefixed with a single byte for the length.
/// \remark The length bytes *includes* the null-terminator, which is not
///         considered for the purposes of reading. Thus, internal null
///         characters are allowed and will not end the read prematurely.
/// \deprecated This function is included for use in existing esp and bsa format
///             code only, it should not be used when you have freedom over the
///             serialization format.
std::string readBzString(std::istream &);

/// Read a non-null-terminated string prefixed with a single byte for the
/// length.
std::string readBString(std::istream &);

/// Write a null-terminated string prefixed with a single byte for the length.
/// \see `io::readBzString`
/// \deprecated This function is included for use in existing esp and bsa format
///             code only, it should not be used when you have freedom over the
///             serialization format.
std::ostream &writeBzString(std::ostream &, const std::string &);

/// Write a non-null-terminated string prefixed with a single byte for the
/// length.
std::ostream &writeBString(std::ostream &, const std::string &);

} // namespace io

#endif //OPENOBL_IO_STRING_HPP
