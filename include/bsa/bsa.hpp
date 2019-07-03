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
///
/// `bsa::BsaReader` acts as a view to a BSA file stored on disk, loading the
/// contained files and folders on-demand. It provides access to the stored
/// files through their hashes and---if provided by the archive---their
/// filenames. It also provides an iterator interface for iterating over all
/// folders and files in the archive, and is fiber-safe.
///
/// \remark The underlying archive is assumed to be persistent and immutable
///         throughout the lifetime of the `bsa::BsaReader`; if the archive is
///         modified or becomes inaccessible in any way, the behaviour is
///         undefined.
///
/// `bsa::BsaReader` provides a highly restricted subset of the
/// *AssociativeContainer* requirement. It is not strictly a *Container* from
/// the standard's point of view because it does not provide direct access to
/// the stored elements and instead return view types. This is similar to
/// `std::vector<bool>`, where the underlying structure in memory does not match
/// what is observed by the user. Additionally, the iterator type of
/// `bsa::BsaReader` is only an *InputIterator*, not a *ForwardIterator* as
/// required by *Container*. Nonetheless, the multipass guarantee is provided.
///
/// \remark Internally `bsa::BsaReader` does not use `std::unordered_map`,
///         despite the fact that we have a hash function, because most of the
///         time we *only* have the hash and there is no way to look up elements
///         using precomputed hash values in C++17.
// C++20: Use std::unordered_map with a custom Hash object, if it's faster.
/// \ingroup OpenOBLBsa
class BsaReader {
 public:
  using iterator = impl::FolderIterator;
  using const_iterator = impl::FolderIterator;
  using difference_type = std::ptrdiff_t;
  using size_type = std::size_t;

  explicit BsaReader(const std::string &filename);

  BsaReader() = delete;
  BsaReader(const BsaReader &) = delete;
  BsaReader &operator=(const BsaReader &) = delete;
  BsaReader(BsaReader &&) = delete;
  BsaReader &operator=(BsaReader &&) = delete;

  /// Returns the uncompressed size in bytes of the given file.
  /// Prefer `FileView::size()` if the file is known to be uncompressed.
  /// \throws std::out_of_range if no such file exists.
  [[nodiscard]] uint32_t
  uncompressedSize(HashResult folderHash, HashResult fileHash) const;
  /// \overload
  [[nodiscard]] uint32_t
  uncompressedSize(std::string folder, std::string file) const;

  /// Returns a `std::istream` to the decompressed data for the given file.
  /// This function is expensive, since it performs disk IO and reads the entire
  /// file into memory whether the file is compressed or not.
  /// \throws std::out_of_range if no such file exists.
  [[nodiscard]] FileData
  stream(HashResult folderHash, HashResult fileHash) const;
  /// \overload
  [[nodiscard]] FileData
  stream(std::string folder, std::string file) const;

  /// Checks whether the given file is present in the archive.
  [[nodiscard]] bool
  contains(HashResult folderHash, HashResult fileHash) const noexcept;
  /// \overload
  [[nodiscard]] bool
  contains(std::string folder, std::string file) const noexcept;

  /// Returns a view to the given file if it exists, and an empty view
  /// otherwise.
  [[nodiscard]] FileView
  getRecord(HashResult folderHash, HashResult fileHash) const noexcept;
  /// \overload
  [[nodiscard]] FileView
  getRecord(std::string folder, std::string file) const noexcept;

  /// Returns the `ArchiveFlag`s describing the underlying archive.
  [[nodiscard]] ArchiveFlag getArchiveFlags() const noexcept;
  /// Returns the `FileType` of files stored in the underlying archive.
  [[nodiscard]] FileType getFileType() const noexcept;

  /// Returns the number of folders in the underlying archive.
  /// \remark The number of files must be queried on a per-folder basis by first
  ///         obtaining a `FolderView` to the desired folder.
  [[nodiscard]] size_type size() const noexcept;
  /// Returns the maximum number of folders that could be stored in a BSA.
  [[nodiscard]] size_type max_size() const noexcept;
  /// Checks whether the underlying archive is empty.
  [[nodiscard]] bool empty() const noexcept;

  /// Returns an iterator the first folder in the underlying archive.
  [[nodiscard]] iterator begin() const noexcept;
  /// Returns an iterator to the first folder in the underlying archive.
  [[nodiscard]] const_iterator cbegin() const noexcept;
  /// Returns an iterator to one-past-the-end of the underlying archive.
  [[nodiscard]] iterator end() const noexcept;
  /// Returns an iterator to one-past-the-end of the underlying archive.
  [[nodiscard]] const_iterator cend() const noexcept;

  /// Returns a view to the given folder, if it exists.
  /// \throws std::out_of_range if no such folder exists.
  FolderView at(HashResult folderHash) const;
  /// \overload
  FolderView at(std::string folder) const;

  /// Returns a view to the given folder. The behaviour is undefined it no such
  /// folder exists.
  [[nodiscard]] FolderView operator[](HashResult folderHash) const noexcept;
  /// \overload
  [[nodiscard]] FolderView operator[](std::string folder) const noexcept;

  /// Checks whether the given folder exists in the archive.
  [[nodiscard]] bool contains(HashResult folderHash) const noexcept;
  /// \overload
  [[nodiscard]] bool contains(std::string folder) const noexcept;

  /// Returns an iterator to the given folder, or one-past-the-end if no such
  /// folder exists.
  [[nodiscard]] const_iterator find(HashResult folderHash) const noexcept;
  /// \overload
  [[nodiscard]] const_iterator find(std::string folder) const noexcept;

  /// \name Header information
  /// Only one format is supported, so these are all hardcoded constants, though
  /// they're still members because they're properties of the specific archive.
  /// @{
  const char *FILE_ID{"BSA"};
  const uint32_t VERSION{0x67};
  const uint32_t OFFSET{0x24};
  /// @}

 private:
  // For impl::FileIterator::internal_iterator
  friend impl::FileIterator;
  // For impl::FolderIterator::internal_iterator
  friend impl::FolderIterator;
  // For FolderView::owner_type
  friend FolderView;
  // For FileView::owner_type
  friend FileView;

  struct FileRecord {
    uint32_t size;
    uint32_t offset;
    std::string name;
    bool compressed;
  };

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

/// A view to a single file in a BSA.
/// \ingroup OpenOBLBsa
class FileView {
 public:
  /// Checks whether the file is empty.
  [[nodiscard]] bool empty() const noexcept;

  /// Returns the name of the file, or an empty view if the underlying archive
  /// does not contain filenames.
  [[nodiscard]] std::string_view name() const noexcept;
  /// Returns the hash of the file.
  [[nodiscard]] HashResult hash() const noexcept;
  /// Checks whether the file is compressed.
  [[nodiscard]] bool compressed() const noexcept;
  /// Returns the compressed size of the file, or the uncompressed size if the
  /// file is not compressed.
  [[nodiscard]] uint32_t size() const noexcept;
  /// Returns the byte offset of the file in the underlying archive.
  [[nodiscard]] uint32_t offset() const noexcept;

  constexpr FileView() noexcept = default;

 private:
  friend BsaReader;
  friend impl::FileIterator;
  friend FolderView;

  using owner_type = BsaReader::FileRecord;
  explicit FileView(HashResult hash, const owner_type *owner) noexcept
      : mHash(hash), mOwner(owner) {}

  HashResult mHash{};
  const owner_type *mOwner{};
};

/// A view to a single folder in a BSA.
/// Acts as a container of `bsa::FileView`s to the files stored within the
/// viewed folder.
/// \remark Like `bsa::BsaReader`, this does not satisfy the *Container*
///         requirement because it returns view types.
/// \ingroup OpenOBLBsa
class FolderView {
 public:
  using value_type = FileView;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using iterator = impl::FileIterator;
  using const_iterator = impl::FileIterator;

  [[nodiscard]] iterator begin() const noexcept;
  [[nodiscard]] iterator end() const noexcept;
  [[nodiscard]] const_iterator cbegin() const noexcept;
  [[nodiscard]] const_iterator cend() const noexcept;

  /// Returns the number of files in the folder.
  [[nodiscard]] size_type size() const noexcept;
  /// Returns the maximum number of files that could be stored in a folder in
  /// a BSA.
  [[nodiscard]] size_type max_size() const noexcept;
  /// Checks whether the folder contains no files.
  [[nodiscard]] bool empty() const noexcept;

  /// Returns a view to the given file, if it exists.
  /// \throws std::out_of_range if no such file exists in the folder.
  [[nodiscard]] FileView at(HashResult fileHash) const;

  /// Returns a view top to the given file. The behaviour is undefined if no
  /// such file exists in the folder.
  [[nodiscard]] FileView operator[](HashResult fileHash) const noexcept;

  /// Returns an iterator to the given file, or one-past-the-end if no such
  /// file exists in the folder.
  [[nodiscard]] iterator find(HashResult fileHash) const noexcept;
  /// Checks whether the given file is contained in the folder.
  [[nodiscard]] bool contains(HashResult fileHash) const noexcept;

  /// Returns the name of the folder, or an empty view if the underlying archive
  /// does not store foldernames.
  [[nodiscard]] std::string_view name() const noexcept;
  /// Returns the hash of the folder.
  [[nodiscard]] HashResult hash() const noexcept;

  constexpr FolderView() noexcept = default;

 private:
  friend BsaReader;
  friend impl::FolderIterator;

  using owner_type = BsaReader::FolderRecord;
  explicit FolderView(HashResult hash, const owner_type *owner) noexcept
      : mHash(hash), mOwner(owner) {}

  HashResult mHash{};
  const owner_type *mOwner{};
};

/// \ingroup OpenOBLBsa
namespace impl {

/// Proxy type for implementing `operator->` for view types.
/// [Thanks Arthur O'Dwyer]
/// (https://quuxplusone.github.io/blog/2019/02/06/arrow-proxy/) for the trick.
template<class Reference>
struct ArrowProxy {
  Reference r;
  Reference *operator->() { return &r; }
};

/// Type to signify construction of an end `FileIterator` or `FolderIterator`.
struct sentinel_tag_t {};
[[maybe_unused]] constexpr static inline sentinel_tag_t sentinel_tag{};

/// *InputIterator* to a `FileView` in a BSA folder.
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

/// *InputIterator* to a `FolderView` in a BSA.
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
