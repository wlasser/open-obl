#include "record/io.hpp"

std::string record::peekRecordType(std::istream &is) {
  // Peek at the next 4 bytes
  char type[5]{};
  is.read(type, 4);
  is.seekg(-4, std::istream::cur);
  return std::string(type);
}

void record::skipRecord(std::istream &is) {
  char type[4]{};
  is.read(type, 4);
  uint32_t size;
  is.read(reinterpret_cast<char *>(&size), 4);
  //std::clog << "Skipping " << type << " (" << 12 + size << ") bytes\n";
  is.seekg(12 + size, std::istream::cur);
}

void record::skipGroup(std::istream &is) {
  char type[4]{};
  is.read(type, 4);
  uint32_t size;
  is.read(reinterpret_cast<char *>(&size), 4);
  // Group size includes the header, unlike records and subrecords
  is.seekg(size - 8, std::istream::cur);
}