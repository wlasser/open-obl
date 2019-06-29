#include "bsa/bsa.hpp"
#include "io/io.hpp"
#include "io/string.hpp"
#include <algorithm>
#include <cctype>
#include <zlib.h>
#include "windows_cleanup.hpp"

namespace bsa {

HashResult genHash(std::string path, HashType type) noexcept {
  // Transform to a lowercase win path
  std::transform(path.begin(), path.end(), path.begin(), [](unsigned char c) {
    return static_cast<unsigned char>(c == '/' ? '\\' : std::tolower(c));
  });

  // Strip trailing slashes from folders and find the file extension of files.
  std::string::iterator extPos;
  if (type == HashType::Folder) {
    while (!path.empty() && path.back() == '\\') path.pop_back();
    extPos = path.end();
  } else {
    const auto dotPos{path.find_last_of('.')};
    extPos = dotPos == std::string::npos ? path.end() : path.begin() + dotPos;
  }
  if (path.empty()) return 0u;

  // Hash the file extension and most of the filename, add the two together,
  // then shift it to the upper four bytes.
  uint64_t h1{impl::sdbmHash(extPos, path.end())};
  uint64_t h2{impl::sdbmHash(path.begin() + 1, extPos - 2)};
  uint64_t hash{(h1 + h2) << 32u};

  // Apply the second part of the hash, ignoring the file extension.
  const auto begin{path.begin()}, end{extPos};
  uint32_t hash2{begin == end ? 0u : static_cast<uint32_t>(*(end - 1))};
  hash2 |= (end - begin <= 2) ? 0u : static_cast<uint32_t>(*(end - 2)) << 8u;
  hash2 |= static_cast<uint32_t>(end - begin) << 16u;
  hash2 |= static_cast<uint32_t>(*begin) << 24u;

  // Modify the hash based on the file extension, if any.
  if (extPos != path.end()) {
    std::string ext(extPos, path.end());
    if (ext == ".kf") hash2 |= 0x80u;
    else if (ext == ".nif") hash2 |= 0x8000u;
    else if (ext == ".dds") hash2 |= 0x8080u;
    else if (ext == ".wav") hash2 |= 0x80000000u;
  }

  return hash + hash2;
}

BsaReader::FolderAccessor
BsaReader::operator[](std::string folder) const {
  return FolderAccessor(bsa::genHash(std::move(folder), HashType::Folder),
                        *this);
}

FileData
BsaReader::FolderAccessor::operator[](std::string file) const {
  return (*this)[bsa::genHash(std::move(file), HashType::File)];
}

FileData
BsaReader::FolderAccessor::operator[](HashResult fileHash) const {
  std::unique_lock lock{mOwner.mMutex};

  const BsaReader::FolderRecord &folder{mOwner.mFolderRecords.at(mHash)};
  const BsaReader::FileRecord &file{folder.files.at(fileHash)};

  // Unset bits higher than the toggle compression bit
  const uint32_t compressedSize{file.size & ~(3u << 30u)};

  // Jump to the data
  mOwner.mIs.seekg(file.offset);

  // Get size of uncompressed data if compressed, otherwise they're the same.
  uint32_t uncompressedSize{compressedSize};
  if (file.compressed) io::readBytes(mOwner.mIs, uncompressedSize);

  // Read data and uncompress if necessary
  std::vector<uint8_t> data(uncompressedSize);
  if (file.compressed) {
    std::vector<unsigned char> compressedData;
    io::readBytes(mOwner.mIs, compressedData, compressedSize);
    unsigned long zlibSize{uncompressedSize};
    uncompress(data.data(), &zlibSize, compressedData.data(), compressedSize);
  } else {
    io::readBytes(mOwner.mIs, data, uncompressedSize);
  }
  return FileData{std::move(data), uncompressedSize};
}

uint32_t BsaReader::FolderAccessor::getSize(HashResult fileHash) const {
  std::unique_lock lock{mOwner.mMutex};

  const BsaReader::FolderRecord &folder{mOwner.mFolderRecords.at(mHash)};
  const BsaReader::FileRecord &file{folder.files.at(fileHash)};

  if (!file.compressed) return file.size;

  // Unset bits higher than the toggle compression bit
  const uint32_t compressedSize{file.size & ~(3u << 30u)};

  // Jump to the data
  mOwner.mIs.seekg(file.offset);
  uint32_t uncompressedSize{compressedSize};
  io::readBytes(mOwner.mIs, uncompressedSize);

  return uncompressedSize;
}

bool BsaReader::readHeader() {
  std::string fileId{};
  io::readBytes(mIs, fileId);
  if (fileId != FILE_ID) return false;

  uint32_t version{};
  io::readBytes(mIs, version);
  if (version != VERSION) return false;

  uint32_t offset{};
  io::readBytes(mIs, offset);
  if (offset != OFFSET) return false;

  io::readBytes(mIs, mArchiveFlags);

  io::readBytes(mIs, mNumFolders);
  io::readBytes(mIs, mNumFiles);
  io::readBytes(mIs, mTotalFolderNameLength);
  io::readBytes(mIs, mTotalFileNameLength);

  io::readBytes(mIs, mFileType);

  return true;
}

std::pair<uint64_t, BsaReader::FileRecord> BsaReader::readFileRecord() {
  HashResult fileHash{};
  uint32_t fileSize{};
  uint32_t fileOffset{};
  io::readBytes(mIs, fileHash);
  io::readBytes(mIs, fileSize);
  io::readBytes(mIs, fileOffset);
  // (1<<30) bit toggles compression of the file from default
  bool compressed{(fileSize & (1u << 30u)) != 0};
  compressed ^= !!(mArchiveFlags & ArchiveFlag::Compressed);
  // TODO: RetainFileNames?
  return {fileHash, FileRecord{fileSize, fileOffset, "", compressed}};
}

std::ifstream::pos_type BsaReader::readFolderRecord() {
  // Read folder record.
  // The offset includes mTotalFileNameLength for some reason.
  HashResult folderHash{};
  uint32_t numFiles{};
  uint32_t folderOffset{};
  io::readBytes(mIs, folderHash);
  io::readBytes(mIs, numFiles);
  io::readBytes(mIs, folderOffset);

  // Record the current position to return to later then jump to the file block.
  const auto pos{mIs.tellg()};
  mIs.seekg(folderOffset - mTotalFileNameLength);

  BsaReader::FolderRecord folderRecord{};

  // Read the folder name if available.
  if (!!(mArchiveFlags & ArchiveFlag::HasDirectoryNames)) {
    std::string path{io::readBzString(mIs)};
    // Transform Win path to *nix path
    std::transform(path.begin(), path.end(), path.begin(), [](char c) -> char {
      return c == '\\' ? '/' : c;
    });
    folderRecord.name = path;
  }

  for (uint32_t j = 0; j < numFiles; ++j) {
    folderRecord.files.emplace(readFileRecord());
  }

  mFolderRecords.emplace(folderHash, std::move(folderRecord));

  // Record the offset to the end of the file block then jump back to the
  // folder block.
  const auto finalOffset{mIs.tellg()};
  mIs.seekg(pos);

  return finalOffset;
}

bool BsaReader::readRecords() {
  // The file record blocks are read during the folder record parse and not
  // after, so we have to jump over them again at the end. To do that we keep
  // track of the largest position in the file that we reach, then jump to that.
  auto largestOffset{mIs.tellg()};

  for (uint32_t i = 0; i < mNumFolders; ++i) {
    largestOffset = std::max(largestOffset, readFolderRecord());
  }

  // Jump past all the file records
  mIs.seekg(largestOffset);
  return true;
}

bool BsaReader::readFileNames() {
  // The file names are listed in the same order as in the BSA, but
  // this is guaranteed to be in increasing order by hash.
  // Conveniently, std::map also stores its elements in increasing
  // hash order.
  for (auto &record : mFolderRecords) {
    for (auto &file : record.second.files) {
      io::readBytes(mIs, file.second.name);
    }
  }
  return true;
}

BsaReader::BsaReader(const std::string &filename)
    : mIs(filename, std::ios_base::in | std::ios_base::binary) {
  if (!mIs.good()) {
    throw std::runtime_error("Failed to open archive '" + filename + "'");
  }
  if (!readHeader()) {
    throw std::runtime_error("Archive '" + filename + "' has invalid header");
  }
  readRecords();
  if (!!(mArchiveFlags & ArchiveFlag::HasFileNames)) {
    readFileNames();
  }
}

bool BsaReader::contains(std::string folder, std::string file) const {
  const auto folderHash{bsa::genHash(std::move(folder), HashType::Folder)};
  const auto folderRecord{mFolderRecords.find(folderHash)};
  if (folderRecord == mFolderRecords.end()) return false;

  const auto fileHash{bsa::genHash(std::move(file), HashType::File)};
  const auto &files{folderRecord->second.files};
  return files.find(fileHash) != files.end();
}

std::optional<BsaReader::FileRecord>
BsaReader::getRecord(std::string folder, std::string file) const {
  const auto folderHash{bsa::genHash(std::move(folder), HashType::Folder)};
  const auto fileHash{bsa::genHash(std::move(file), HashType::File)};
  return getRecord(folderHash, fileHash);
}

std::optional<BsaReader::FileRecord>
BsaReader::getRecord(HashResult folderHash, HashResult fileHash) const {
  const auto folderRecord{mFolderRecords.find(folderHash)};
  if (folderRecord == mFolderRecords.end()) return std::nullopt;

  const auto &files{folderRecord->second.files};
  const auto fileRecord{files.find(fileHash)};
  if (fileRecord == files.end()) return std::nullopt;

  return fileRecord->second;
}

BsaReader::iterator BsaReader::begin() const {
  return BsaReader::iterator{mFolderRecords.begin()};
}

BsaReader::iterator BsaReader::end() const {
  return BsaReader::iterator{mFolderRecords.end(), true};
}

ArchiveFlag BsaReader::getArchiveFlags() const noexcept {
  return mArchiveFlags;
}

FileType BsaReader::getFileType() const noexcept {
  return mFileType;
}

namespace impl {

BsaIterator::reference BsaIterator::updateCurrentPublicRecord() const {
  mCurrentPublicRec.name = mCurrentRec->second.name;
  mCurrentPublicRec.files.clear();
  for (const auto&[hash, record] : mCurrentRec->second.files) {
    mCurrentPublicRec.files.push_back(record.name);
  }
  return mCurrentPublicRec;
}

BsaIterator::reference BsaIterator::operator*() const {
  updateCurrentPublicRecord();
  return mCurrentPublicRec;
}

BsaIterator::pointer BsaIterator::operator->() const {
  updateCurrentPublicRecord();
  return &mCurrentPublicRec;
}

BsaIterator &BsaIterator::operator++() {
  ++mCurrentRec;
  return *this;
}

const BsaIterator BsaIterator::operator++(int) {
  auto tmp = *this;
  ++mCurrentRec;
  return tmp;
}

BsaIterator &BsaIterator::operator--() {
  --mCurrentRec;
  return *this;
}

const BsaIterator BsaIterator::operator--(int) {
  auto tmp = *this;
  --mCurrentRec;
  return tmp;
}

bool BsaIterator::operator==(const BsaIterator &other) {
  return mCurrentRec == other.mCurrentRec;
}

} // namespace impl

} // namespace bsa
