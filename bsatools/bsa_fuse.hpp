#ifndef OPENOBLIVION_BSATOOLS_BSA_FUSE_HPP
#define OPENOBLIVION_BSATOOLS_BSA_FUSE_HPP

#include "bsa/bsa.hpp"
#include <optional>
#include <string>

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

class BsaContext {
 private:
  BsaReader mBsaReader;
  std::unique_ptr<FolderNode> mRoot;

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
};

const BsaContext &
getBsaContext(std::optional<std::string> filename = std::nullopt);

} // namespace bsa

#endif // OPENOBLIVION_BSATOOLS_BSA_FUSE_HPP
