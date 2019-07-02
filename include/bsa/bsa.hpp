#ifndef OPENOBL_BSA_BSA_HPP
#define OPENOBL_BSA_BSA_HPP

#include "io/memstream.hpp"
#include <boost/fiber/mutex.hpp>
#include <fstream>
#include <iterator>
#include <map>
#include <optional>
#include <string>
#include <vector>

/// \file bsa.hpp
/// \defgroup OpenOBLBsa Bsa
/// Provides read-only access to BSA files.

namespace bsa {

// Using a nested namespace here really confused Doxygen, which ends up putting
// everything below in a bsa::bsa namespace.
namespace impl {
class FileIterator;
class FolderIterator;
} // namespace impl

class FolderView;
class FileView;

/// `std::istream` interface to the uncompressed data of a file in a BSA.
/// \ingroup OpenOBLBsa
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
/// \ingroup OpenOBLBsa
enum class HashType : int {
  File = 0,
  Folder
};

/// The result of applying the `genHash` function to a path.
// This is just to provide semantic information in the interface, in the
// implementation where bit manipulation is involved it is probably clearer to
// use `uint64_t` directly.
/// \ingroup OpenOBLBsa
using HashResult = uint64_t;

/// Hash the given file or folder name as determined by the `HashType`.
/// \remark Uses the algorithm described in
///         https://en.uesp.net/wiki/Tes4Mod:Hash_Calculation
/// \ingroup OpenOBLBsa
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
/// \ingroup OpenOBLBsa
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
/// \ingroup OpenOBLBsa
constexpr inline ArchiveFlag operator|(ArchiveFlag a, ArchiveFlag b) noexcept {
  return static_cast<ArchiveFlag>(static_cast<uint32_t>(a)
      | static_cast<uint32_t>(b));
}
/// \ingroup OpenOBLBsa
constexpr inline ArchiveFlag operator&(ArchiveFlag a, ArchiveFlag b) noexcept {
  return static_cast<ArchiveFlag>(static_cast<uint32_t>(a)
      & static_cast<uint32_t>(b));
}
/// \ingroup OpenOBLBsa
constexpr inline bool operator!(ArchiveFlag a) noexcept {
  return static_cast<uint32_t>(a) == 0;
}

/// The type of file stored in a BSA file.
/// It is intended that all the files in a BSA file be semantically related,
/// though this is not necessary and nothing is said about the file extensions
/// of the stored files.
/// \ingroup OpenOBLBsa
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
/// \ingroup OpenOBLBsa
class BsaReader {
 public:
  using iterator = impl::FolderIterator;
  using const_iterator = impl::FolderIterator;
  using difference_type = std::ptrdiff_t;
  using size_type = std::size_t;

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
  const_iterator cbegin() const;
  const_iterator cend() const;

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

  ArchiveFlag getArchiveFlags() const noexcept;
  FileType getFileType() const noexcept;

 private:
  friend impl::FileIterator;
  friend impl::FolderIterator;
  friend FolderView;
  friend FileView;

  using FileRecordMap = std::map<HashResult, FileRecord>;

  struct FolderRecord {
    std::string name;
    FileRecordMap files;
  };

  using FolderRecordMap = std::map<HashResult, FolderRecord>;

  FolderRecordMap mFolderRecords;
  mutable std::ifstream mIs;
  mutable boost::fibers::mutex mMutex{};

  ArchiveFlag mArchiveFlags{ArchiveFlag::None};
  FileType mFileType{FileType::None};
  uint32_t mNumFolders{};
  uint32_t mNumFiles{};
  // Total length of all folder names, including null-terminators but not
  // including prefixed length bytes.
  uint32_t mTotalFolderNameLength{};
  uint32_t mTotalFileNameLength{};

  bool readHeader();
  std::pair<HashResult, BsaReader::FileRecord> readFileRecord();
  std::ifstream::pos_type readFolderRecord();
  bool readRecords();
  bool readFileNames();
};

// A FileView is a view to a file.
class FileView {
 public:
  [[nodiscard]] bool empty() const noexcept;

  [[nodiscard]] std::string_view name() const noexcept;
  [[nodiscard]] HashResult hash() const noexcept;
  [[nodiscard]] bool compressed() const noexcept;
  [[nodiscard]] uint32_t size() const noexcept;
  [[nodiscard]] uint32_t offset() const noexcept;

 private:
  friend impl::FileIterator;
  friend FolderView;

  using owner_type = BsaReader::FileRecord;
  explicit FileView(HashResult hash, const owner_type *owner) noexcept
      : mHash(hash), mOwner(owner) {}

  HashResult mHash{};
  const owner_type *mOwner{};
};

// A FolderView is a view to a folder. It is a Container of FileViews.
class FolderView {
 public:
//  using key_type = HashResult;
//  using mapped_type = FileView;
//  using value_type = std::pair<const HashResult, FileView>;
  using value_type = FileView;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
//  using reference = const value_type &;
//  using const_reference = const value_type &;
  using iterator = impl::FileIterator;
  using const_iterator = impl::FileIterator;

  [[nodiscard]] iterator begin() const noexcept;
  [[nodiscard]] iterator end() const noexcept;
  [[nodiscard]] const_iterator cbegin() const noexcept;
  [[nodiscard]] const_iterator cend() const noexcept;

  [[nodiscard]] size_type size() const noexcept;
  [[nodiscard]] size_type max_size() const noexcept;
  [[nodiscard]] bool empty() const noexcept;

  [[nodiscard]] FileView at(HashResult fileHash) const;
  [[nodiscard]] FileView operator[](HashResult fileHash) const noexcept;

  [[nodiscard]] iterator find(HashResult fileHash) const noexcept;
  [[nodiscard]] bool contains(HashResult fileHash) const noexcept;

  [[nodiscard]] std::string_view name() const noexcept;
  [[nodiscard]] HashResult hash() const noexcept;

 private:
  friend BsaReader;
  friend impl::FolderIterator;

  using owner_type = BsaReader::FolderRecord;
  explicit FolderView(HashResult hash, const owner_type *owner) noexcept
      : mHash(hash), mOwner(owner) {}

  HashResult mHash{};
  const owner_type *mOwner{};
};

namespace impl {

// Thanks Arthur O'Dwyer
// https://quuxplusone.github.io/blog/2019/02/06/arrow-proxy/
template<class Reference>
struct ArrowProxy {
  Reference r;
  Reference *operator->() { return &r; }
};

struct sentinel_tag_t {};
[[maybe_unused]] constexpr static inline sentinel_tag_t sentinel_tag{};

class FileIterator {
 private:
  using internal_iterator = BsaReader::FileRecordMap::const_iterator;

 public:
  using value_type = FileView;
  using difference_type = std::ptrdiff_t;
  using reference = FileView;
  using pointer = ArrowProxy<reference>;
  // Limited to InputIterator because we have to return FileView by value.
  using iterator_category = std::input_iterator_tag;

  friend bool operator==(const FileIterator &a, const FileIterator &b) noexcept;

  reference operator*() const noexcept;
  pointer operator->() const noexcept;

  FileIterator &operator++() noexcept;
  const FileIterator operator++(int) noexcept;

 private:
  friend FolderView;

  internal_iterator mIt;
  bool mIsSentinel;

  explicit FileIterator(internal_iterator it) noexcept
      : mIt(it), mIsSentinel(false) {}
  explicit FileIterator(sentinel_tag_t) noexcept : mIsSentinel(true) {}
};

inline bool operator==(const FileIterator &a, const FileIterator &b) noexcept {
  if (a.mIsSentinel && b.mIsSentinel) return true;
  if (a.mIsSentinel || b.mIsSentinel) return false;
  return a.mIt == b.mIt;
}

inline bool operator!=(const FileIterator &a, const FileIterator &b) noexcept {
  return !(a == b);
}

class FolderIterator {
 private:
  using internal_iterator = BsaReader::FolderRecordMap::const_iterator;

 public:
  using value_type = FolderView;
  using difference_type = std::ptrdiff_t;
  using reference = FolderView;
  using pointer = ArrowProxy<reference>;
  // Limited to InputIterator because we have to return FolderView by value.
  using iterator_category = std::input_iterator_tag;

  friend bool
  operator==(const FolderIterator &a, const FolderIterator &b) noexcept;

  reference operator*() const noexcept;
  pointer operator->() const noexcept;

  FolderIterator &operator++() noexcept;
  const FolderIterator operator++(int) noexcept;

 private:
  friend BsaReader;

  internal_iterator mIt;
  bool mIsSentinel;

  explicit FolderIterator(internal_iterator it) noexcept
      : mIt(it), mIsSentinel(false) {}
  explicit FolderIterator(sentinel_tag_t) noexcept : mIsSentinel(true) {}
};

inline bool
operator==(const FolderIterator &a, const FolderIterator &b) noexcept {
  if (a.mIsSentinel && b.mIsSentinel) return true;
  if (a.mIsSentinel || b.mIsSentinel) return false;
  return a.mIt == b.mIt;
}

inline bool
operator!=(const FolderIterator &a, const FolderIterator &b) noexcept {
  return !(a == b);
}

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

#endif // OPENOBL_BSA_BSA_HPP
