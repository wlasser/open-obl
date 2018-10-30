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

namespace bsa {

// TODO: Use std::filesystem::path
uint64_t genHash(std::string path, HashType hashType) {
  uint64_t hash{0u};
  uint32_t hash2{0u};

  // Transform to a lowercase win path
  std::string str{path};
  std::transform(path.begin(), path.end(), str.begin(),
                 [](unsigned char c) {
                   return static_cast<unsigned char>(std::tolower(
                       c == '/' ? '\\' : c));
                 });

  // Presence of a . does not imply a file e.g. /foo.bar/baz is a
  // valid folder. Moreover folders should not have a trailing slash, if they do
  // then we ignore it.
  if (hashType == HashType::Folder && str.back() == '\\') str.pop_back();
  const auto begin{str.begin()};
  auto end{str.end()};

  // Empty string hashes to zero
  if (str.begin() == str.end()) return 0u;

  const auto extPos =
      (hashType == HashType::Folder ? end : begin + str.find_last_of('.'));

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
  hash <<= 32u;

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

BsaReader::FolderAccessor
BsaReader::operator[](std::string folder) const {
  return FolderAccessor(genHash(std::move(folder), HashType::Folder), *this);
}

FileData
BsaReader::FolderAccessor::operator[](std::string file) const {
  return (*this)[genHash(std::move(file), HashType::File)];
}

FileData
BsaReader::FolderAccessor::operator[](uint64_t fileHash) const {
  std::unique_lock lock{owner.isMutex};

  const FolderRecord &folder{owner.folderRecords.at(hash)};
  const FileRecord &file{folder.files.at(fileHash)};

  // Unset bits higher than the toggle compression bit
  const uint32_t compressedSize{file.size & ~(3u << 30u)};

  // Jump to the data
  owner.is.seekg(file.offset);

  // Skip over full path if given
  if (!!(owner.archiveFlags & ArchiveFlag::RetainFileNames)) {
    io::readBString(owner.is);
  }

  // Get size of uncompressed data if compressed, otherwise they're the same.
  uint32_t uncompressedSize{compressedSize};
  if (file.compressed) {
    io::readBytes(owner.is, uncompressedSize);
  }

  // Read data and uncompress if necessary
  std::vector<uint8_t> data(uncompressedSize);
  if (file.compressed) {
    std::vector<unsigned char> compressedData(compressedSize);
    if (!io::safeRead(owner.is, compressedData.data(), compressedSize)) {
      throw io::IOReadError("compressed data", owner.is.rdstate());
    }
    unsigned long zlibSize{uncompressedSize};
    uncompress(data.data(), &zlibSize, compressedData.data(), compressedSize);
  } else {
    if (!io::safeRead(owner.is, data.data(), uncompressedSize)) {
      throw io::IOReadError("uncompressed data", owner.is.rdstate());
    }
  }
  return FileData{std::move(data), uncompressedSize};
}

bool BsaReader::readHeader() {
  std::string fileId{};
  io::readBytes(is, fileId);
  if (fileId != FILE_ID) return false;

  uint32_t version{};
  io::readBytes(is, version);
  if (version != VERSION) return false;

  uint32_t offset{};
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

bool BsaReader::readRecords() {
  // The file record blocks are read during the folder record parse and not
  // after, so we have to jump over them again at the end. To do that we keep
  // track of the largest position in the file that we reach, then jump to that.
  auto largestOffset{is.tellg()};

  for (uint32_t i = 0; i < folderCount; ++i) {
    // Read folder record
    uint64_t hash{};
    uint32_t count{};
    // This includes totalFileNameLength for some reason
    uint32_t offset{};
    io::readBytes(is, hash);
    io::readBytes(is, count);
    io::readBytes(is, offset);

    // Jump to file record block
    FolderRecord folderRecord{};
    const auto pos{is.tellg()};

    // Correct the offset
    is.seekg(offset - totalFileNameLength);

    // Read folder name if available
    if (!!(archiveFlags & ArchiveFlag::HasDirectoryNames)) {
      auto path{io::readBzString(is)};
      folderRecord.name = path;
      // Transform Win path to *nix path
      std::transform(path.begin(), path.end(), folderRecord.name.begin(),
                     [](unsigned char c) { return c == '\\' ? '/' : c; });
    }

    for (uint32_t j = 0; j < count; ++j) {
      // Read file record
      uint64_t fileHash{};
      uint32_t fileSize{};
      uint32_t fileOffset{};
      io::readBytes(is, fileHash);
      io::readBytes(is, fileSize);
      io::readBytes(is, fileOffset);
      // (1<<30) bit toggles compression of the file from default
      bool compressed{(fileSize & (1u << 30u)) != 0};
      compressed ^= !!(archiveFlags & ArchiveFlag::Compressed);
      // TODO: RetainFileNames?
      folderRecord.files.emplace(fileHash, FileRecord{
          fileSize, fileOffset, "", compressed});
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

bool BsaReader::readFileNames() {
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

BsaReader::BsaReader(std::string filename) : is(filename) {
  if (!is.good()) {
    throw std::runtime_error(boost::str(
        boost::format("Failed to open archive '%s'") % filename));
  }
  if (!readHeader()) {
    throw std::runtime_error(boost::str(
        boost::format("Archive '%s' has invalid header") % filename));
  }
  readRecords();
  if (!!(archiveFlags & ArchiveFlag::HasFileNames)) {
    readFileNames();
  }
}

bool BsaReader::contains(std::string folder, std::string file) const {
  const auto folderHash{genHash(std::move(folder), HashType::Folder)};
  const auto folderRecord{folderRecords.find(folderHash)};
  if (folderRecord == folderRecords.end()) return false;

  const auto fileHash{genHash(std::move(file), HashType::File)};
  const auto &files{folderRecord->second.files};
  return files.find(fileHash) != files.end();
}

std::optional<BsaReader::FileRecord>
BsaReader::getRecord(std::string folder, std::string file) const {
  const auto folderHash{genHash(std::move(folder), HashType::Folder)};
  const auto fileHash{genHash(std::move(file), HashType::File)};
  return getRecord(folderHash, fileHash);
}

std::optional<BsaReader::FileRecord>
BsaReader::getRecord(uint64_t folderHash, uint64_t fileHash) const {
  const auto folderRecord{folderRecords.find(folderHash)};
  if (folderRecord == folderRecords.end()) return std::nullopt;

  const auto &files{folderRecord->second.files};
  const auto fileRecord{files.find(fileHash)};
  if (fileRecord == files.end()) return std::nullopt;

  return fileRecord->second;
}

BsaReader::iterator BsaReader::begin() const {
  return BsaReader::iterator{folderRecords.begin()};
}

BsaReader::iterator BsaReader::end() const {
  return BsaReader::iterator{folderRecords.end(), true};
}

namespace impl {

BsaIterator::reference BsaIterator::updateCurrentPublicRecord() const {
  currentPublicRecord.name = currentRecord->second.name;
  currentPublicRecord.files.clear();
  for (const auto&[hash, record] : currentRecord->second.files) {
    currentPublicRecord.files.push_back(record.name);
  }
  return currentPublicRecord;
}

BsaIterator::reference BsaIterator::operator*() const {
  updateCurrentPublicRecord();
  return currentPublicRecord;
}

BsaIterator::pointer BsaIterator::operator->() const {
  updateCurrentPublicRecord();
  return &currentPublicRecord;
}

BsaIterator &BsaIterator::operator++() {
  ++currentRecord;
  return *this;
}

const BsaIterator BsaIterator::operator++(int) {
  auto tmp = *this;
  ++currentRecord;
  return tmp;
}

BsaIterator &BsaIterator::operator--() {
  --currentRecord;
  return *this;
}

const BsaIterator BsaIterator::operator--(int) {
  auto tmp = *this;
  --currentRecord;
  return tmp;
}

bool BsaIterator::operator==(const BsaIterator &other) {
  return currentRecord == other.currentRecord;
}

} // namespace impl

} // namespace bsa
