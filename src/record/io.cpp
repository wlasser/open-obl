#include "record/io.hpp"
#include <gsl/gsl>

uint32_t record::peekRecordType(std::istream &is) noexcept {
  // Ensure that the stream is always returned to its original position.
  const auto startPos{is.tellg()};
  const auto seekGuard = gsl::finally([&]() {
    is.clear();
    is.seekg(startPos, std::ios_base::beg);
  });

  uint32_t type{};
  try {
    io::readBytes(is, type);
  } catch (const io::IOReadError &e) {
    return 0;
  }

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
  io::readBytes(is, type);

  uint32_t size;
  io::readBytes(is, size);

  // Group size includes the header, unlike records and subrecords
  is.seekg(size - 8, std::istream::cur);
}