#ifndef OPENOBLIVION_BSA_HPP
#define OPENOBLIVION_BSA_HPP

#include <fstream>
#include <string>
#include <memory>
#include <map>

#include "io/memstream.hpp"

namespace bsa {

class FileData : public io::memstream {
 private:
  std::unique_ptr<uint8_t[]> ptr;
  std::size_t l;
 public:
  std::size_t size() { return l; }
  FileData(std::unique_ptr<uint8_t[]> p, std::size_t l) :
      memstream(p.get(), l), ptr(std::move(p)), l(l) {}
};

uint64_t genHash(const std::string &, bool);

// TODO: Allow const access
// TODO: Allow std::string access
// TODO: Update implementation to use io::readBytes over io::safeRead
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
    const std::unique_ptr<FileData> operator[](const char *file) const;
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

  std::map<uint64_t, FolderRecord> folderRecords;
  mutable std::ifstream is;
  bool readHeader();
  bool readRecords();
  bool readFileNames();

 public:
  explicit BSAReader(const char *);

  inline FolderAccessor operator[](uint64_t hash) const {
    return FolderAccessor(hash, *this);
  }
  FolderAccessor operator[](const char *) const;
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
}

#endif // OPENOBLIVION_BSA_HPP
