#ifndef OPENOBLIVION_BSA_BSA_HPP
#define OPENOBLIVION_BSA_BSA_HPP

#include <algorithm>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "io/memstream.hpp"

namespace bsa {

namespace impl {

class BsaIterator;

} // namespace impl

class FileData : public io::memstream {
 private:
  std::unique_ptr<uint8_t[]> ptr;
  std::size_t l;
 public:
  std::size_t size() { return l; }

  FileData(std::unique_ptr<uint8_t[]> p, std::size_t l) noexcept :
      memstream(p.get(), l), ptr(std::move(p)), l(l) {}

  FileData(FileData &&other) noexcept :
      FileData(std::move(other.ptr), other.l) {}
};

uint64_t genHash(std::string, bool);

class BsaReader {
 public:
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

  enum class FileFlag : uint32_t {
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
  const char FILE_ID[4] = "BSA";
  const uint32_t VERSION = 0x67;
  const uint32_t OFFSET = 0x24;
  ArchiveFlag archiveFlags;
  uint32_t folderCount;
  uint32_t fileCount;
  // Total length of all folder names, including null-terminators but not
  // including prefixed length bytes.
  uint32_t totalFolderNameLength;
  uint32_t totalFileNameLength;
  FileFlag fileFlags;

 private:

  using RecordMap = std::map<uint64_t, FolderRecord>;
  RecordMap folderRecords;
  mutable std::ifstream is;
  bool readHeader();
  bool readRecords();
  bool readFileNames();

 public:
  explicit BsaReader(std::string);

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

inline BsaReader::ArchiveFlag operator|(BsaReader::ArchiveFlag a,
                                        BsaReader::ArchiveFlag b) {
  return static_cast<BsaReader::ArchiveFlag>(static_cast<uint32_t>(a)
      | static_cast<uint32_t>(b));
}
inline BsaReader::ArchiveFlag operator&(BsaReader::ArchiveFlag a,
                                        BsaReader::ArchiveFlag b) {
  return static_cast<BsaReader::ArchiveFlag>(static_cast<uint32_t>(a)
      & static_cast<uint32_t>(b));
}

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
