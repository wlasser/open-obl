#ifndef OPENOBLIVION_STRING_HPP
#define OPENOBLIVION_STRING_HPP

#include <string>
#include <istream>

namespace io {

// Read a null-terminated string prefixed with a single byte for the length.
std::string readBzString(std::istream &);

// Read a string prefixed with a single byte for the length, without a
// null-terminator.
std::string readBString(std::istream &);

} // namespace io

#endif //OPENOBLIVION_STRING_HPP
