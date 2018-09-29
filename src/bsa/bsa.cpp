#include "bsa/bsa.hpp"
#include "io/io.hpp"
#include "io/read_bytes.hpp"
#include "io/string.hpp"
#include <boost/format.hpp>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <memory>
#include <string>
#include <zlib.h>

// TODO: Use std::filesystem::path
uint64_t bsa::genHash(std::string path, bool isFolder) {
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
  // valid folder. Moreover folders should not have a trailing slash, if they do
  // then we ignore it.
  if (isFolder && str.back() == '\\') str.pop_back();
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
bsa::BSAReader::operator[](std::string folder) const {
  return bsa::BSAReader::FolderAccessor(bsa::genHash(std::move(folder), true),
                                        *this);
}

bsa::FileData
bsa::BSAReader::FolderAccessor::operator[](std::string file) const {
  return (*this)[bsa::genHash(std::move(file), false)];
}

bsa::FileData
bsa::BSAReader::FolderAccessor::operator[](uint64_t fileHash) const {
  const FolderRecord &folder = owner.folderRecords.at(hash);
  const FileRecord &file = folder.files.at(fileHash);
  // Unset bits higher than the toggle compression bit
  uint32_t size = file.size & ~(3u << 30u);
  uint32_t uncompressedSize = size;
  // Jump to the data
  owner.is.seekg(file.offset);
  // Skip over full path if given
  if (static_cast<bool>(owner.archiveFlags
      & bsa::BSAReader::ArchiveFlag::RetainFileNames)) {
    io::readBString(owner.is);
  }
  // Get size of uncompressed data if compressed
  if (file.compressed) {
    io::readBytes(owner.is, uncompressedSize);
  }
  // Read data and uncompress if necessary
  auto data = std::make_unique<uint8_t[]>(uncompressedSize);
  if (file.compressed) {
    // Blame ZLib for all the casts. unsigned char[] is required for the data,
    // but char* is required for reading. unsigned long is required for the
    // uncompressed size, but is > 32 bits on some systems, and the BSA requires
    // a 32 bit uncompressed size.
    auto compressedData = std::make_unique<unsigned char[]>(size);
    owner.is.read(reinterpret_cast<char *>(compressedData.get()), size);
    auto zlibUncompressedSize = static_cast<unsigned long>(uncompressedSize);
    uncompress(data.get(), &zlibUncompressedSize, compressedData.get(), size);
  } else {
    owner.is.read(reinterpret_cast<char *>(data.get()), size);
  }
  return bsa::FileData{std::move(data), uncompressedSize};
}

bool bsa::BSAReader::readHeader() {
  char fileId[4]{};
  if (!io::safeRead(is, fileId, 4) || strcmp(fileId, FILE_ID) != 0)
    return false;

  uint32_t version = 0;
  io::readBytes(is, version);
  if (version != VERSION) return false;

  uint32_t offset = 0;
  io::readBytes(is, offset);
  if (offset != OFFSET) return false;

  io::readBytes(is, archiveFlags);

  io::readBytes(is, folderCount);
  io::readBytes(is, fileCount);
  io::readBytes(is, totalFolderNameLength);
  io::readBytes(is, totalFileNameLength);

  io::readBytes(is, fileFlags);

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
    io::readBytes(is, hash);
    io::readBytes(is, count);
    // This includes totalFileNameLength for some reason, so it
    // will need to be subtracted
    io::readBytes(is, offset);

    // Jump to file record block
    FolderRecord folderRecord = {};
    auto pos = is.tellg();
    is.seekg(offset - totalFileNameLength);

    // Read folder name if available
    if (static_cast<bool>(archiveFlags & ArchiveFlag::HasDirectoryNames)) {
      auto path = io::readBzString(is);
      folderRecord.name = path;
      // Transform Win path to *nix path
      std::transform(path.begin(), path.end(), folderRecord.name.begin(),
                     [](unsigned char c) { return c == '\\' ? '/' : c; });
    }

    for (uint32_t j = 0; j < count; ++j) {
      // Read file record
      uint64_t fileHash = 0;
      uint32_t fileSize = 0;
      uint32_t fileOffset = 0;
      io::readBytes(is, fileHash);
      io::readBytes(is, fileSize);
      io::readBytes(is, fileOffset);
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
      io::readBytes(is, file.second.name);
    }
  }
  return true;
}

bsa::BSAReader::BSAReader(std::string filename) : is(filename) {
  if (!is.good()) {
    throw std::runtime_error(boost::str(
        boost::format("Failed to open archive '%s'") % filename));
  }
  readHeader();
  readRecords();
  if (static_cast<bool>(archiveFlags & ArchiveFlag::HasFileNames)) {
    readFileNames();
  }
}

bool bsa::BSAReader::contains(std::string folder, std::string file) const {
  auto folderHash = bsa::genHash(std::move(folder), true);
  auto folderRecord = folderRecords.find(folderHash);
  if (folderRecord == folderRecords.end()) return false;

  auto fileHash = bsa::genHash(std::move(file), false);
  const auto &files = folderRecord->second.files;
  return files.find(fileHash) != files.end();
}

std::optional<bsa::BSAReader::FileRecord>
bsa::BSAReader::getRecord(std::string folder, std::string file) const {
  auto folderHash = bsa::genHash(std::move(folder), true);
  auto fileHash = bsa::genHash(std::move(file), false);
  return getRecord(folderHash, fileHash);
}

std::optional<bsa::BSAReader::FileRecord>
bsa::BSAReader::getRecord(uint64_t folderHash, uint64_t fileHash) const {
  auto folderRecord = folderRecords.find(folderHash);
  if (folderRecord == folderRecords.end()) return std::nullopt;

  const auto &files = folderRecord->second.files;
  auto fileRecord = files.find(fileHash);
  if (fileRecord == files.end()) return std::nullopt;

  return fileRecord->second;
}

bsa::BSAReader::iterator bsa::BSAReader::begin() const {
  return bsa::BSAReader::iterator{folderRecords.begin()};
}

bsa::BSAReader::iterator bsa::BSAReader::end() const {
  return bsa::BSAReader::iterator{folderRecords.end(), true};
}

namespace bsa::impl {

BSAIterator::reference BSAIterator::updateCurrentPublicRecord() const {
  currentPublicRecord.name = currentRecord->second.name;
  currentPublicRecord.files.clear();
  for (const auto&[hash, record] : currentRecord->second.files) {
    currentPublicRecord.files.push_back(record.name);
  }
  return currentPublicRecord;
}

BSAIterator::reference BSAIterator::operator*() const {
  updateCurrentPublicRecord();
  return currentPublicRecord;
}

BSAIterator::pointer BSAIterator::operator->() const {
  updateCurrentPublicRecord();
  return &currentPublicRecord;
}

BSAIterator &BSAIterator::operator++() {
  ++currentRecord;
  return *this;
}

const BSAIterator BSAIterator::operator++(int) {
  auto tmp = *this;
  ++currentRecord;
  return tmp;
}

BSAIterator &BSAIterator::operator--() {
  --currentRecord;
  return *this;
}

const BSAIterator BSAIterator::operator--(int) {
  auto tmp = *this;
  --currentRecord;
  return tmp;
}

bool BSAIterator::operator==(const BSAIterator &other) {
  return currentRecord == other.currentRecord;
}

} // namespace bsa::impl