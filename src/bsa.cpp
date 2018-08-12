#include <cstring>
#include <fstream>
#include <memory>
#include <cctype>
#include <string>
#include <algorithm>
#include <zlib.h>

#include "bsa.hpp"
#include "io_util.hpp"

uint64_t bsa::genHash(const std::string &path, bool isFolder) {
  uint64_t hash = 0;
  uint32_t hash2 = 0;
  // Transform to a lowercase win path
  std::string str(path);
  std::transform(path.begin(), path.end(), str.begin(),
                 [](unsigned char c) -> unsigned char {
                   return static_cast<unsigned char>(std::tolower(
                       c == '/' ? '\\' : c));
                 });
  // Presence of a . does not imply a file e.g. /foo.bar/baz is a
  // valid folder
  const auto begin = str.begin();
  auto end = str.end();
  const auto extPos = (isFolder ? end : begin + str.find_last_of('.'));
  // Empty string hashes to zero
  if (str.begin() == str.end()) return 0;

  // Hash 1
  // Hash the file extension (if it exists) then strip it
  for (auto c = extPos; c < end; ++c) {
    hash = (hash * 0x1003f) + static_cast<uint8_t>(*c);
  }
  end = extPos;
  for (auto c = begin + 1; c < end - 2; ++c) {
    hash2 = (hash2 * 0x1003f) + static_cast<uint8_t>(*c);
  }
  hash += hash2;
  hash <<= 32;

  // Hash 2
  hash2 = static_cast<uint32_t>(*(end - 1));
  hash2 |= (end - begin > 2) ? static_cast<uint8_t>(*(end - 2)) << 8u : 0;
  hash2 |= static_cast<uint8_t>(end - begin) << 16u;
  hash2 |= static_cast<uint8_t>(*begin) << 24u;

  if (extPos != str.end()) {
    std::string ext(extPos, str.end());
    if (ext == ".kf") hash2 |= 0x80;
    else if (ext == ".nif") hash2 |= 0x8000;
    else if (ext == ".dds") hash2 |= 0x8080;
    else if (ext == ".wav") hash2 |= 0x80000000;
  }

  return hash + hash2;
}

bsa::BSAReader::FolderAccessor
bsa::BSAReader::operator[](const char *folder) {
  return bsa::BSAReader::FolderAccessor(bsa::genHash(std::string(folder), true),
                                        *this);
}

std::unique_ptr<bsa::FileData>
bsa::BSAReader::FolderAccessor::operator[](const char *file) {
  return (*this)[bsa::genHash(std::string(file), false)];
}

std::unique_ptr<bsa::FileData>
bsa::BSAReader::FolderAccessor::operator[](uint64_t fileHash) {
  const FolderRecord &folder = owner.folderRecords.at(hash);
  const FileRecord &file = folder.files.at(fileHash);
  // Unset bits higher than the toggle compression bit
  uint32_t size = file.size & ~(3u << 30u);
  unsigned long uncompressedSize = size;
  // Jump to the data
  owner.is.seekg(file.offset);
  // Skip over full path if given
  if (static_cast<bool>(owner.archiveFlags
      & bsa::BSAReader::ArchiveFlag::RetainFileNames)) {
    readBString(owner.is);
  }
  // Get size of uncompressed data if compressed
  if (file.compressed) {
    owner.is.read(reinterpret_cast<char *>(&uncompressedSize), 4);
  }
  // Read data and uncompress if necessary
  auto data = new uint8_t[uncompressedSize];
  if (file.compressed) {
    auto compressedData = new unsigned char[size];
    owner.is.read(reinterpret_cast<char *>(compressedData), size);
    uncompress(data, &uncompressedSize, compressedData, size);
  } else {
    owner.is.read(reinterpret_cast<char *>(data), size);
  }
  return std::make_unique<bsa::FileData>(std::unique_ptr<uint8_t>(data),
                                         uncompressedSize);
}

bool bsa::BSAReader::readHeader() {
  char fileId[4]{};
  if (!safeRead(is, fileId, 4) || strcmp(fileId, FILE_ID) != 0) return false;

  uint32_t version = 0;
  if (!safeRead(is, &version, 4) || version != VERSION) return false;

  uint32_t offset = 0;
  if (!safeRead(is, &offset, 4) || offset != OFFSET) return false;

  uint32_t flags = 0;
  if (!safeRead(is, &flags, 4)) return false;
  archiveFlags = static_cast<ArchiveFlag>(flags & 127u);

  if (!safeRead(is, &folderCount, 4)) return false;
  if (!safeRead(is, &fileCount, 4)) return false;
  if (!safeRead(is, &totalFolderNameLength, 4)) return false;
  if (!safeRead(is, &totalFileNameLength, 4)) return false;

  flags = 0;
  if (!safeRead(is, &flags, 4)) return false;
  fileFlags = static_cast<FileFlag>(flags & 511u);

  return true;
}

bool bsa::BSAReader::readRecords() {
  // The file record blocks are read during the folder record parse
  // and not after, so we have to jump over them again at the end. To
  // do that we keep track of the largest position in the file that we
  // reach, then jump to that.
  auto largestOffset = is.tellg();

  for (uint32_t i = 0; i < folderCount; ++i) {
    // Read folder record
    uint64_t hash = 0;
    uint32_t count = 0;
    uint32_t offset = 0;
    if (!safeRead(is, &hash, 8)) return false;
    if (!safeRead(is, &count, 4)) return false;
    // This include totalFileNameLength for some reason, so it
    // will need to be subtracted
    if (!safeRead(is, &offset, 4)) return false;

    // Jump to file record block
    FolderRecord folderRecord = {};
    auto pos = is.tellg();
    is.seekg(offset - totalFileNameLength);

    // Read folder name if available
    if (static_cast<bool>(archiveFlags & ArchiveFlag::HasDirectoryNames)) {
      folderRecord.name = readBzString(is);
    }

    for (uint32_t j = 0; j < count; ++j) {
      // Read file record
      uint64_t fileHash = 0;
      uint32_t fileSize = 0;
      uint32_t fileOffset = 0;
      if (!safeRead(is, &fileHash, 8)) return false;
      if (!safeRead(is, &fileSize, 4)) return false;
      if (!safeRead(is, &fileOffset, 4)) return false;
      // (1<<30) bit toggles compression of the file from default
      bool compressed = (fileSize & (1u << 30u)) != 0;
      compressed ^= static_cast<bool>(archiveFlags & ArchiveFlag::Compressed);
      folderRecord.files.emplace(fileHash, FileRecord{fileSize,
                                                      fileOffset, "",
                                                      compressed});
    }

    // Add the folder
    folderRecords.emplace(hash, std::move(folderRecord));
    // Record offset
    if (is.tellg() > largestOffset) largestOffset = is.tellg();
    // Jump back to file record block
    is.seekg(pos);
  }

  // Jump past all the file records
  is.seekg(largestOffset);
  return true;
}

bool bsa::BSAReader::readFileNames() {
  // The file names are listed in the same order as in the BSA, but
  // this is guaranteed to be in increasing order by hash.
  // Conveniently, std::map also stores its elements in increasing
  // hash order.
  for (auto &record : folderRecords) {
    for (auto &file : record.second.files) {
      // File names are null-terminated strings. There might be
      // a better way of doing this but for now seek to the next
      // null to find the length, allocate, jump back, then read.
      auto pos = is.tellg();
      while (is.get() != '\0') {}
      auto len = is.tellg() - pos;
      is.seekg(pos);
      auto str = new char[len];
      is.read(str, len);
      file.second.name = std::string(str);
      delete[] str;
    }
  }
  return true;
}

bsa::BSAReader::BSAReader(const char *filename) : is(filename) {
  readHeader();
  readRecords();
  if (static_cast<bool>(archiveFlags & ArchiveFlag::HasFileNames)) {
    readFileNames();
  }
}
