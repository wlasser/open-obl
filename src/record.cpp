#include "record.hpp"

std::string record::peekRecordType(std::istream &is) {
  // Peek at the next 4 bytes
  char type[5]{};
  is.read(type, 4);
  is.seekg(-4, std::istream::cur);
  return std::string(type);
}
