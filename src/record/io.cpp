#include "record/io.hpp"
#include <array>
#include <string>

uint32_t record::peekRecordType(std::istream &is) {
  uint32_t type{};
  is.read(reinterpret_cast<char *>(&type), sizeof(type));
  if (is.rdstate() != std::ios::goodbit) return 0;
  is.seekg(-4, std::istream::cur);
  return type;
}

record::RecordHeader record::readRecordHeader(std::istream &is) {
  record::RecordHeader result{};
  is.read(reinterpret_cast<char *>(&result), 16);
  // Skip the versionControlInfo we're ignoring
  is.seekg(4, std::istream::cur);
  return result;
}

[[maybe_unused]] record::RecordHeader record::skipRecord(std::istream &is) {
  auto header = readRecordHeader(is);
  is.seekg(header.size, std::ios_base::cur);
  return header;
}

void record::skipGroup(std::istream &is) {
  std::array<char, 4> type{};
  is.read(type.data(), 4);
  uint32_t size;
  is.read(reinterpret_cast<char *>(&size), 4);
  // Group size includes the header, unlike records and subrecords
  is.seekg(size - 8, std::istream::cur);
}