#ifndef OPENOBLIVION_BSA_HPP
#define OPENOBLIVION_BSA_HPP

#include <algorithm>
#include <iterator>
#include <fstream>
#include <string>
#include <memory>
#include <map>
#include <vector>

#include "io/memstream.hpp"

namespace bsa {

namespace impl {

class BSAIterator;

} // namespace impl

class FileData : public io::memstream {
 private:
  std::unique_ptr<uint8_t[]> ptr;
  std::size_t l;
 public:
  std::size_t size() { return l; }
  FileData(std::unique_ptr<uint8_t[]> p, std::size_t l) :
      memstream(p.get(), l), ptr(std::move(p)), l(l) {}
};

uint64_t genHash(std::string, bool);

class BSAReader {
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

  using iterator = impl::BSAIterator;
  friend impl::BSAIterator;

 private:

  typedef struct {
    uint32_t size;
    uint32_t offset;
    std::string name;
    bool compressed;
  } FileRecord;

  typedef struct {
    std::string name;
    std::map<uint64_t, FileRecord> files;
  } FolderRecord;

  class FolderAccessor {
    friend BSAReader;
   private:
    uint64_t hash;
    const BSAReader &owner;
    FolderAccessor(uint64_t hash, const BSAReader &owner) :
        hash(hash), owner(owner) {}
   public:
    const std::unique_ptr<FileData> operator[](uint64_t) const;
    const std::unique_ptr<FileData> operator[](std::string file) const;
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
  explicit BSAReader(std::string);

  bool contains(std::string folder, std::string file) const;

  iterator begin() const;
  iterator end() const;

  inline FolderAccessor operator[](uint64_t hash) const {
    return FolderAccessor(hash, *this);
  }
  FolderAccessor operator[](std::string) const;
};

inline BSAReader::ArchiveFlag operator|(BSAReader::ArchiveFlag a,
                                        BSAReader::ArchiveFlag b) {
  return static_cast<BSAReader::ArchiveFlag>(static_cast<uint32_t>(a)
      | static_cast<uint32_t>(b));
}
inline BSAReader::ArchiveFlag operator&(BSAReader::ArchiveFlag a,
                                        BSAReader::ArchiveFlag b) {
  return static_cast<BSAReader::ArchiveFlag>(static_cast<uint32_t>(a)
      & static_cast<uint32_t>(b));
}

// Public version of BSAReader::FolderRecord, for iterating.
// I am unsatisfied with how the filenames are duplicated in memory between
// these records and the existing private ones.
struct FolderRecord {
  std::string name;
  std::vector<std::string> files;
};

namespace impl {

class BSAIterator {
 public:
  // Iterator traits
  using value_type = bsa::FolderRecord;
  using difference_type = std::ptrdiff_t;
  using pointer = value_type *const;
  using reference = const value_type &;
  using iterator_category = std::bidirectional_iterator_tag;

  explicit BSAIterator(BSAReader::RecordMap::const_iterator currentRecord,
                       bool isEnd = false) : currentRecord(currentRecord) {
    if (!isEnd) updateCurrentPublicRecord();
  }

  reference operator*() const;
  pointer operator->() const;

  BSAIterator &operator++();
  const BSAIterator operator++(int);
  BSAIterator &operator--();
  const BSAIterator operator--(int);

  bool operator==(const BSAIterator &rhs);
  bool operator!=(const BSAIterator &rhs) {
    return !operator==(rhs);
  }

 private:
  BSAReader::RecordMap::const_iterator currentRecord{};
  mutable bsa::FolderRecord currentPublicRecord{};

  reference updateCurrentPublicRecord() const;
};

} // namespace impl
} // namespace bsa

#endif // OPENOBLIVION_BSA_HPP
