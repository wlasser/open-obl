#include "record/io.hpp"
#include <gsl/gsl>
#include <zlib.h>

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
  } catch (const io::IOReadError &) {
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

std::vector<uint8_t> record::compressBytes(const std::vector<uint8_t> &uncomp) {
  const auto uncompSize{gsl::narrow_cast<unsigned long>(uncomp.size())};
  unsigned long compSize{::compressBound(uncompSize)};

  std::vector<uint8_t> comp(compSize);
  ::compress(comp.data(), &compSize, uncomp.data(), uncompSize);
  comp.resize(compSize);

  return comp;
}

std::vector<uint8_t> record::uncompressBytes(const std::vector<uint8_t> &comp,
                                             std::size_t uncompSize) {
  std::vector<uint8_t> uncomp(uncompSize);
  auto lUncompSize{gsl::narrow_cast<unsigned long>(uncompSize)};
  const auto compSize{gsl::narrow_cast<unsigned long>(comp.size())};

  ::uncompress(uncomp.data(), &lUncompSize, comp.data(), compSize);

  return uncomp;
}
