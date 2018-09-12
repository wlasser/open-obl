#include "record/io.hpp"

std::string record::peekRecordType(std::istream &is) {
  // Peek at the next 4 bytes
  char type[5]{};
  is.read(type, 4);
  is.seekg(-4, std::istream::cur);
  return std::string(type);
}

record::RecordHeader record::readRecordHeader(std::istream &is) {
  record::RecordHeader result{};
  is.read(reinterpret_cast<char *>(&result), 16);
  // Skip the versionControlInfo we're ignoring
  is.seekg(4, std::istream::cur);
  return result;
}

record::RecordHeader record::skipRecord(std::istream &is) {
  auto header = readRecordHeader(is);
  is.seekg(header.size, std::ios_base::cur);
  return header;
}

void record::skipGroup(std::istream &is) {
  char type[4]{};
  is.read(type, 4);
  uint32_t size;
  is.read(reinterpret_cast<char *>(&size), 4);
  // Group size includes the header, unlike records and subrecords
  is.seekg(size - 8, std::istream::cur);
}