#ifndef OPENOBLIVION_BSATOOLS_BSA_FUSE_HPP
#define OPENOBLIVION_BSATOOLS_BSA_FUSE_HPP

#include "bsa/bsa.hpp"
#include <optional>
#include <string>
#include <utility>

namespace bsa {

class FolderNode;
class FileNode;

/// Base class for entries in the filesystem tree.
/// \see `bsa::FolderNode`
/// \see `bsa::FileNode`
class Node {
 public:
  virtual ~Node() = 0;

  /// Whether this node represents a folder or a file.
  [[nodiscard]] virtual bool isFolder() const noexcept = 0;

  /// Get a pointer to the folder containing this node.
  /// \warning The parent of the root node in the filesystem tree is `nullptr`,
  ///          not itself, unlike in POSIX.
  [[nodiscard]] virtual FolderNode *getParent() const noexcept = 0;

  /// The unqualified name of the folder or file represented by this node.
  [[nodiscard]] virtual std::string getName() const = 0;
};
inline Node::~Node() = default;

/// Represents a folder entry in the filesystem tree.
/// A folder has a name, a parent, and a set of children `bsa::Node`s
/// representing those filesystem entries contained within the folder.
/// The special folders `.` and `..` are not considered, and are treated as any
/// other file.
/// \see `oo::Path`
class FolderNode : public Node {
 private:
  /// The unqualified name of the folder.
  std::string mName{};

  /// The parent of the folder. `nullptr` if this is the root of the tree.
  FolderNode *mParent{};

  /// Filesystem entries contained within this folder.
  std::vector<std::unique_ptr<Node>> mChildren{};

 public:
  FolderNode(std::string name, FolderNode *parent)
      : mName(std::move(name)), mParent(parent) {}

  [[nodiscard]] bool isFolder() const noexcept override {
    return true;
  }

  [[nodiscard]] FolderNode *getParent() const noexcept override {
    return mParent;
  }

  [[nodiscard]] std::string getName() const override {
    return mName;
  }

  /// Return a pointer to the child folder with the given unqualified `name`,
  /// if one exists, and `nullptr` otherwise.
  /// Only direct children are considered, there is no recursive lookup.
  FolderNode *findChildFolder(const std::string &name);

  /// Return a pointer to the child file with the given unqualified `name`,
  /// if one exists, and `nullptr` otherwise.
  /// Only direct children are considered, there is no recursive lookup.
  FileNode *findChildFile(const std::string &name);

  /// Add a folder with the given `name` as a child of this node, if one doesn't
  /// exist.
  /// \returns a pointer to the added folder, or a pointer to the existing one
  ///          if this folder already has a child folder with the given `name`.
  FolderNode *addChildFolder(std::string name);

  /// Add a file representing the given file record add a child of this node, if
  /// one with the same name as `rec` doesn't exist.
  /// \returns a pointer to the added file, or a pointer to the existing one if
  ///          this folder already has a child file whose name matches the name
  ///          of `rec`.
  FileNode *addChildFile(const BsaReader::FileRecord &rec);

  /// Return a vector of non-owning pointers to the children of this folder.
  /// \todo Replace this with iterators.
  std::vector<Node *> getChildren() const;
};

/// Represents a file entry in the filesystem tree.
/// A file has a name, a size (in bytes), a compression flag, and a parent
/// folder.
class FileNode : public Node {
 private:
  std::string mName{};
  uint32_t mSize{0};
  bool mCompressed{false};
  FolderNode *mParent{};

 public:
  FileNode(const BsaReader::FileRecord &rec, FolderNode *parent)
      : mName(rec.name), mSize(rec.size), mCompressed(rec.compressed),
        mParent(parent) {}

  [[nodiscard]] bool isFolder() const noexcept override {
    return false;
  }

  [[nodiscard]] FolderNode *getParent() const noexcept override {
    return mParent;
  }

  [[nodiscard]] std::string getName() const override {
    return mName;
  }

  /// The size of the file contents in bytes.
  [[nodiscard]] uint32_t getSize() const noexcept {
    return mSize;
  }

  /// Whether the file is stored in a compressed format or not.
  [[nodiscard]] bool isCompressed() const noexcept {
    return mCompressed;
  }
};

/// Stores the hashes of a file and folder pair.
/// Used for keeping track of open files worrying about filename normalization.
/// \see `bsa::genHash()`.
struct BsaHashPair {
  uint64_t first;
  uint64_t second;

  BsaHashPair(std::string fst, std::string sec)
      : first(bsa::genHash(std::move(fst), bsa::HashType::Folder)),
        second(bsa::genHash(std::move(sec), bsa::HashType::File)) {}

  friend bool operator<(const BsaHashPair &lhs,
                        const BsaHashPair &rhs) noexcept {
    return lhs.first < rhs.first
        || (lhs.first == rhs.first && lhs.second < rhs.second);
  }
};

/// Owning context for a single bsa archive and its filesystem tree.
class BsaContext {
 private:
  BsaReader mBsaReader;

  /// Root folder of the filesystem tree.
  std::unique_ptr<FolderNode> mRoot;

  /// Map of open files to a `std::istream` for accessing its content.
  std::map<BsaHashPair, bsa::FileData> mOpenFiles{};

 public:
  /// Open the bsa file with the given `filename` and build a filesystem tree of
  /// its entries.
  /// \see `bsa::BsaReader(std::string)`
  explicit BsaContext(std::string filename);

  /// Get a pointer to the root folder of the filesystem tree.
  [[nodiscard]] FolderNode *getRoot() const noexcept {
    return mRoot.get();
  }

  /// Get a handle to the underlying Bsa file itself.
  const BsaReader &getReader() const noexcept {
    return mBsaReader;
  }

  /// Get a pointer to the folder with the given fully-qualified name, if one
  /// exists, and `nullptr` otherwise.
  [[nodiscard]] FolderNode *findFolder(std::string foldername) const;

  /// Get a pointer to the file with the given fully-qualified name, if one
  /// exists, and `nullptr` otherwise.
  [[nodiscard]] Node *findEntry(std::string filename) const;

  /// Open the given file for reading, if it exists.
  /// Does nothing if the file is already open.
  /// \returns `-ENOENT` if the file could not be opened, and zero otherwise.
  int open(std::string folder, std::string file);

  /// Close the given file, if it is open, otherwise do nothing.
  int close(std::string folder, std::string file);

  /// Whether the given file is open for reading or not.
  bool isOpen(std::string folder, std::string file);

  /// Return a standard stream to the given open file.
  /// \pre `isOpen(folder, file)`.
  std::istream &getStream(std::string folder, std::string file)
  /*C++20: [[expects: isOpen(folder, file)]]*/;
};

/// Return `oo::Path::folder()` and `oo::Path::filename()` of the given `path`.
std::pair<std::string, std::string> splitPath(const char *path);

/// \overload `bsa::splitPath(const char*)`
std::pair<std::string, std::string> splitPath(std::string &&path);

/// Return a reference to the `bsa::BsaContext`.
/// The first call to this function should have a non-null `filename`, which is
/// used to construct a `bsa::BsaContext`. Subsequent calls return a reference
/// to the previously constructed `bsa::BsaContext`, and should omit the
/// argument.
///
/// This is a glorified singleton, and is used because libfuse expects function
/// pointers defining the filesystem operations. Lambdas with captures don't
/// decay to function pointers, and the expected types don't allow for user data
/// to be passed in, so we are stuck with globals. I think the low-level fuse
/// operations allow user data in some of the methods, so perhaps this could be
/// avoided if it's *really* desired. On the other hand, each execution of the
/// program loads at most one Bsa file which exists for the entire duration of
/// the program, so a singleton is hardly the worst fit.
BsaContext &getBsaContext(std::optional<std::string> filename = std::nullopt);

} // namespace bsa

#endif // OPENOBLIVION_BSATOOLS_BSA_FUSE_HPP
