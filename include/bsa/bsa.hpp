#ifndef OPENOBLIVION_BSA_BSA_HPP
#define OPENOBLIVION_BSA_BSA_HPP

#include "io/memstream.hpp"
#include <fstream>
#include <iterator>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

/// \file bsa.hpp
/// \defgroup OpenOblivionBsa Bsa
/// Provides read-only access to BSA files.

namespace bsa {

// Using a nested namespace here really confused Doxygen, which ends up putting
// everything below in a bsa::bsa namespace.
namespace impl {
class BsaIterator;
} // namespace impl

/// `std::istream` interface to the uncompressed data of a file in a BSA.
/// \ingroup OpenOblivionBsa
class FileData : public io::memstream {
 private:
  std::vector<uint8_t> mData;
  std::size_t mSize;
 public:
  std::size_t size() const noexcept { return mSize; }

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

/// Signifies whether a path to an entry in a BSA file is to a file or a folder.
/// This information must be known when hashing the path, because the hashing
/// algorithm differs in each case.
/// \ingroup OpenOblivionBsa
enum class HashType : int {
  File = 0,
  Folder
};

/// The result of applying the `genHash` function to a path.
// This is just to provide semantic information in the interface, in the
// implementation where bit manipulation is involved it is probably clearer to
// use `uint64_t` directly.
/// \ingroup OpenOblivionBsa
using HashResult = uint64_t;

/// Hash the given file or folder name as determined by the `HashType`.
/// \remark Uses the algorithm described in
///         https://en.uesp.net/wiki/Tes4Mod:Hash_Calculation
/// \ingroup OpenOblivionBsa
HashResult genHash(std::string path, HashType type) noexcept;

/// Flags describing the structure of a BSA file.
/// <table>
/// <tr><th>Flag Name</th><th>Flag Description</th></tr>
/// <tr><td>`HasDirectoryNames`</td>
///     <td>The name of each folder is stored in the archive in addition to its
///         hash.</tr></tr>
/// <tr><td>`HasFileNames`</td>
///     <td>The name of each file is stored in the archive in addition to its
///         hash.</td></tr>
/// <tr><td>`Compressed`</td>
///     <td>All the files in the archive are compressed using ZLib compression.
///         This is transparent; as a user you do not need to perform the
///         decompression manually. The main consideration is that if this flag
///         is set, then `bsa::FileRecord` stores the *compressed* size of the
///         file. The function `bsa::FolderRecord::getSize(HashResult)` will
///         always return the *uncompressed* size of the file with the given
///         hash.</td></tr>
/// <tr><td>`RetainDirectoryNames`</td><td>Unused.</td></tr>
/// <tr><td>`RetainFileNames`</td><td>Unused.</td></tr>
/// <tr><td>`RetainOffsets`</td><td>Unused.</td></tr>
/// <tr><td>`BigEndian`</td>
///     <td>Unused. All archives are assumed to be little endian.</td></tr>
/// </table>
/// \ingroup OpenOblivionBsa
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
/// \ingroup OpenOblivionBsa
constexpr inline ArchiveFlag operator|(ArchiveFlag a, ArchiveFlag b) noexcept {
  return static_cast<ArchiveFlag>(static_cast<uint32_t>(a)
      | static_cast<uint32_t>(b));
}
/// \ingroup OpenOblivionBsa
constexpr inline ArchiveFlag operator&(ArchiveFlag a, ArchiveFlag b) noexcept {
  return static_cast<ArchiveFlag>(static_cast<uint32_t>(a)
      & static_cast<uint32_t>(b));
}
/// \ingroup OpenOblivionBsa
constexpr inline bool operator!(ArchiveFlag a) noexcept {
  return static_cast<uint32_t>(a) == 0;
}

/// The type of file stored in a BSA file.
/// It is intended that all the files in a BSA file be semantically related,
/// though this is not necessary and nothing is said about the file extensions
/// of the stored files.
/// \ingroup OpenOblivionBsa
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

/// Provides read-only access to a BSA file.
/// \ingroup OpenOblivionBsa
class BsaReader {
 public:
  using iterator = impl::BsaIterator;
  friend impl::BsaIterator;

  struct FileRecord {
    uint32_t size;
    uint32_t offset;
    std::string name;
    bool compressed;
  };

  class FolderAccessor {
   public:
    FileData operator[](HashResult fileHash) const;
    FileData operator[](std::string file) const;
    uint32_t getSize(HashResult fileHash) const;

   private:
    friend BsaReader;
    HashResult mHash;
    const BsaReader &mOwner;
    FolderAccessor(HashResult hash, const BsaReader &owner) :
        mHash(hash), mOwner(owner) {}
  };

  explicit BsaReader(const std::string &filename);

  BsaReader() = delete;
  BsaReader(const BsaReader &) = delete;
  BsaReader &operator=(const BsaReader &) = delete;
  BsaReader(BsaReader &&) = delete;
  BsaReader &operator=(BsaReader &&) = delete;

  bool contains(std::string folder, std::string file) const;

  std::optional<FileRecord>
  getRecord(std::string folder, std::string file) const;
  std::optional<FileRecord>
  getRecord(HashResult folderHash, HashResult fileHash) const;

  iterator begin() const;
  iterator end() const;

  inline FolderAccessor operator[](HashResult hash) const {
    return FolderAccessor(hash, *this);
  }
  FolderAccessor operator[](std::string) const;

  /// \name Header information
  /// Only one format is supported, so these are all hardcoded constants, though
  /// they're still members because they're properties of the specific archive.
  /// @{
  const char *FILE_ID{"BSA"};
  const uint32_t VERSION{0x67};
  const uint32_t OFFSET{0x24};
  /// @}
  ArchiveFlag archiveFlags{ArchiveFlag::None};
  uint32_t folderCount{};
  uint32_t fileCount{};
  // Total length of all folder names, including null-terminators but not
  // including prefixed length bytes.
  uint32_t totalFolderNameLength{};
  uint32_t totalFileNameLength{};
  FileType fileFlags{};

 private:
  struct FolderRecord {
    std::string name;
    std::map<HashResult, FileRecord> files;
  };

  using RecordMap = std::map<HashResult, FolderRecord>;
  RecordMap folderRecords;
  mutable std::ifstream is;
  mutable std::mutex isMutex{};
  bool readHeader();
  std::pair<HashResult, BsaReader::FileRecord> readFileRecord();
  std::ifstream::pos_type readFolderRecord();
  bool readRecords();
  bool readFileNames();
};

/// Public version of `BsaReader::FolderRecord`, for iterating.
/// \todo I am unsatisfied with how the filenames are duplicated in memory
///       between these records and the existing private ones. Can we get away
///       with storing `std::string_view`s here?
/// \ingroup OpenOblivionBsa
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
  using pointer = const value_type *;
  using reference = const value_type &;
  using iterator_category = std::bidirectional_iterator_tag;

  explicit BsaIterator(BsaReader::RecordMap::const_iterator currentRecord,
                       bool isEnd = false) : mCurrentRec(currentRecord) {
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
  BsaReader::RecordMap::const_iterator mCurrentRec{};
  mutable bsa::FolderRecord mCurrentPublicRec{};

  reference updateCurrentPublicRecord() const;
};

/// Compute the sdbm hash of the given range.
/// \remark The *LessThanComparable* requirement is so that things like
///         `sdbmHash(str.begin() + 1, str.end())` return zero for empty `str`.
template<class InputIt, class = std::enable_if_t<
    std::is_base_of_v<std::input_iterator_tag,
                      typename std::iterator_traits<InputIt>::iterator_category>
        && std::is_convertible_v<
            typename std::iterator_traits<InputIt>::value_type, HashResult>
        && std::is_convertible_v<
            decltype(std::declval<InputIt>() < std::declval<InputIt>()), bool>>>
constexpr HashResult sdbmHash(InputIt first, InputIt last) noexcept {
  // This is equal to 2^16 + 2^6 - 1 and is prime, though according to
  // http://www.cse.yorku.ca/~oz/hash.html that's accidental.
  constexpr uint64_t MAGIC{65599u};
  HashResult h{0u};
  for (auto it = first; it < last; ++it) {
    h = h * MAGIC + static_cast<uint64_t>(*it);
  }
  return h;
}

} // namespace impl
} // namespace bsa

#endif // OPENOBLIVION_BSA_BSA_HPP
