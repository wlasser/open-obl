#include "io/read_bytes.hpp"

template<>
void io::readBytes(std::istream &is, std::string &data) {
  std::getline(is, data, '\0');
  if (is.rdstate() != std::ios::goodbit) {
    throw IOReadError("std::string", is.rdstate());
  }
}

