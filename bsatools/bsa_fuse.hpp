#ifndef OPENOBLIVION_BSATOOLS_BSA_FUSE_HPP
#define OPENOBLIVION_BSATOOLS_BSA_FUSE_HPP

#include "bsa/bsa.hpp"
#include <optional>
#include <string>
#include <utility>

namespace bsa {

class FolderNode;
class FileNode;

class Node {
 public:
  virtual ~Node() = 0;

  [[nodiscard]] virtual bool isFolder() const noexcept = 0;
  [[nodiscard]] virtual FolderNode *getParent() const noexcept = 0;
  [[nodiscard]] virtual std::string getName() const = 0;
};
inline Node::~Node() = default;

class FolderNode : public Node {
 private:
  std::string mName{};
  FolderNode *mParent{};
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

  FolderNode *findChildFolder(const std::string &name);
  FileNode *findChildFile(const std::string &name);

  FolderNode *addChildFolder(std::string name);
  FileNode *addChildFile(const BsaReader::FileRecord &rec);

  // TODO: Replace with iterators
  std::vector<Node *> getChildren() const;
};

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

  [[nodiscard]] uint32_t getSize() const noexcept {
    return mSize;
  }

  [[nodiscard]] bool isCompressed() const noexcept {
    return mCompressed;
  }
};

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

class BsaContext {
 private:
  BsaReader mBsaReader;
  std::unique_ptr<FolderNode> mRoot;

  std::map<BsaHashPair, bsa::FileData> mOpenFiles{};

 public:
  BsaContext(std::string filename);

  [[nodiscard]] FolderNode *getRoot() const noexcept {
    return mRoot.get();
  }

  const BsaReader &getReader() const noexcept {
    return mBsaReader;
  }

  [[nodiscard]] FolderNode *findFolder(std::string foldername) const;
  [[nodiscard]] Node *findEntry(std::string filename) const;

  int open(std::string folder, std::string file);
  int close(std::string folder, std::string file);
  bool isOpen(std::string folder, std::string file);
  std::istream &getStream(std::string folder, std::string file);
};

std::pair<std::string, std::string> splitPath(const char *path);
std::pair<std::string, std::string> splitPath(std::string &&path);

BsaContext &
getBsaContext(std::optional<std::string> filename = std::nullopt);

} // namespace bsa

#endif // OPENOBLIVION_BSATOOLS_BSA_FUSE_HPP
