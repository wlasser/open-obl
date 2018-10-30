#ifndef OPENOBLIVION_BSA_BSA_HPP
#define OPENOBLIVION_BSA_BSA_HPP

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "io/memstream.hpp"

namespace bsa::impl {
class BsaIterator;
} // namespace bsa::impl

namespace bsa {

class FileData : public io::memstream {
 private:
  std::vector<uint8_t> mData;
  std::size_t mSize;
 public:
  std::size_t size() { return mSize; }

  FileData(std::vector<uint8_t> data, std::size_t l) noexcept :
      memstream(data.data(), l), mData(std::move(data)), mSize(l) {}

  FileData(const FileData &other) = delete;
  FileData &operator=(const FileData &other) = delete;

  FileData(FileData &&other) noexcept :
      FileData(std::move(other.mData), other.mSize) {}
  FileData &operator=(FileData &&other) noexcept {
    std::swap(*this, other);
    return *this;
  }
};

enum class HashType : int {
  File = 0,
  Folder
};

uint64_t genHash(std::string, HashType);

enum class ArchiveFlag : uint32_t {
  None = 0u,
  HasDirectoryNames = 1u << 0u,
  HasFileNames = 1u << 1u,
  Compressed = 1u << 2u,
  RetainDirectoryNames = 1u << 3u,
  RetainFileNames = 1u << 4u,
  RetainOffsets = 1u << 5u,
  BigEndian = 1u << 6u
};
constexpr inline ArchiveFlag operator|(ArchiveFlag a, ArchiveFlag b) {
  return static_cast<ArchiveFlag>(static_cast<uint32_t>(a)
      | static_cast<uint32_t>(b));
}
constexpr inline ArchiveFlag operator&(ArchiveFlag a, ArchiveFlag b) {
  return static_cast<ArchiveFlag>(static_cast<uint32_t>(a)
      & static_cast<uint32_t>(b));
}
constexpr inline bool operator!(ArchiveFlag a) {
  return static_cast<uint32_t>(a) == 0;
}

enum class FileType : uint32_t {
  None = 0u,
  Meshes = 1u << 0u,
  Textures = 1u << 1u,
  Menus = 1u << 2u,
  Sounds = 1u << 3u,
  Voices = 1u << 4u,
  Shaders = 1u << 5u,
  Trees = 1u << 6u,
  Fonts = 1u << 7u,
  Misc = 1u << 8u
};

class BsaReader {
 public:
  using iterator = impl::BsaIterator;
  friend impl::BsaIterator;

  typedef struct {
    uint32_t size;
    uint32_t offset;
    std::string name;
    bool compressed;
  } FileRecord;

 private:

  typedef struct {
    std::string name;
    std::map<uint64_t, FileRecord> files;
  } FolderRecord;

  class FolderAccessor {
    friend BsaReader;
   private:
    uint64_t hash;
    const BsaReader &owner;
    FolderAccessor(uint64_t hash, const BsaReader &owner) :
        hash(hash), owner(owner) {}
   public:
    FileData operator[](uint64_t) const;
    FileData operator[](std::string file) const;
  };

 public:
  /// Header information
  const char *FILE_ID{"BSA"};
  const uint32_t VERSION{0x67};
  const uint32_t OFFSET{0x24};
  ArchiveFlag archiveFlags{ArchiveFlag::None};
  uint32_t folderCount{};
  uint32_t fileCount{};
  // Total length of all folder names, including null-terminators but not
  // including prefixed length bytes.
  uint32_t totalFolderNameLength{};
  uint32_t totalFileNameLength{};
  FileType fileFlags{};

 private:
  using RecordMap = std::map<uint64_t, FolderRecord>;
  RecordMap folderRecords;
  mutable std::ifstream is;
  bool readHeader();
  bool readRecords();
  bool readFileNames();

 public:
  explicit BsaReader(std::string);

  BsaReader() = delete;
  BsaReader(const BsaReader &) = delete;
  BsaReader &operator=(const BsaReader &) = delete;
  BsaReader(BsaReader &&) = delete;
  BsaReader &operator=(BsaReader &&) = delete;

  bool contains(std::string folder, std::string file) const;

  std::optional<FileRecord> getRecord(std::string folder,
                                      std::string file) const;
  std::optional<FileRecord> getRecord(uint64_t folderHash,
                                      uint64_t fileHash) const;

  iterator begin() const;
  iterator end() const;

  inline FolderAccessor operator[](uint64_t hash) const {
    return FolderAccessor(hash, *this);
  }
  FolderAccessor operator[](std::string) const;
};

// Public version of BsaReader::FolderRecord, for iterating.
// I am unsatisfied with how the filenames are duplicated in memory between
// these records and the existing private ones.
struct FolderRecord {
  std::string name;
  std::vector<std::string> files;
};

namespace impl {

class BsaIterator {
 public:
  // Iterator traits
  using value_type = bsa::FolderRecord;
  using difference_type = std::ptrdiff_t;
  using pointer = value_type *const;
  using reference = const value_type &;
  using iterator_category = std::bidirectional_iterator_tag;

  explicit BsaIterator(BsaReader::RecordMap::const_iterator currentRecord,
                       bool isEnd = false) : currentRecord(currentRecord) {
    if (!isEnd) updateCurrentPublicRecord();
  }

  reference operator*() const;
  pointer operator->() const;

  BsaIterator &operator++();
  const BsaIterator operator++(int);
  BsaIterator &operator--();
  const BsaIterator operator--(int);

  bool operator==(const BsaIterator &rhs);
  bool operator!=(const BsaIterator &rhs) {
    return !operator==(rhs);
  }

 private:
  BsaReader::RecordMap::const_iterator currentRecord{};
  mutable bsa::FolderRecord currentPublicRecord{};

  reference updateCurrentPublicRecord() const;
};

} // namespace impl
} // namespace bsa

#endif // OPENOBLIVION_BSA_BSA_HPP
