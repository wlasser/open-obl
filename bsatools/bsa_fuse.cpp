#include "bsa_fuse.hpp"
#include "fuse.hpp"
#include "fuse_operations.hpp"
#include "fuse_options.hpp"
#include "fs/path.hpp"
#include <algorithm>
#include <iostream>
#include <string>
#include <string_view>

int main(int argc, char *argv[]) {
  fuser::Args args{argc, argv, 0};
  bsa::CmdOptConf conf{};

  fuser::optParse(&args, &conf, bsa::fuseCmdOpts.data(), bsa::handleCmdOpts);

  if (!conf.archivePath) {
    std::cerr << "please provide an archive parameter\n";
    std::exit(1);
  }

  // Construct the global bsa context
  try {
    bsa::getBsaContext(conf.archivePath);
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    std::exit(1);
  }

  return fuser::main(args.argc, args.argv, bsa::fuseOps);
}

bsa::FolderNode *bsa::FolderNode::findChildFolder(const std::string &name) {
  auto pred = [&name](const auto &child) {
    return child->isFolder() && child->getName() == name;
  };
  auto it{std::find_if(mChildren.begin(), mChildren.end(), pred)};
  return it != mChildren.end() ? static_cast<FolderNode *>(it->get()) : nullptr;
}

bsa::FileNode *bsa::FolderNode::findChildFile(const std::string &name) {
  auto pred = [&name](const auto &child) {
    return !child->isFolder() && child->getName() == name;
  };
  auto it{std::find_if(mChildren.begin(), mChildren.end(), pred)};
  return it != mChildren.end() ? static_cast<FileNode *>(it->get()) : nullptr;
}

bsa::FolderNode *bsa::FolderNode::addChildFolder(std::string name) {
  if (auto *childNode{findChildFolder(name)}) return childNode;

  auto folderNode{std::make_unique<FolderNode>(std::move(name), this)};
  const auto &node{mChildren.emplace_back(std::move(folderNode))};
  return static_cast<FolderNode *>(node.get());
}

bsa::FileNode *
bsa::FolderNode::addChildFile(bsa::FileView rec) {
  if (auto *childNode{findChildFile(std::string(rec.name()))}) return childNode;

  auto fileNode{std::make_unique<FileNode>(rec, this)};
  const auto &node{mChildren.emplace_back(std::move(fileNode))};
  return static_cast<FileNode *>(node.get());
}

bsa::FileNode *
bsa::FolderNode::addChildFile(bsa::FileView rec, uint32_t uncompressedSize) {
  if (auto *childNode{findChildFile(std::string(rec.name()))}) return childNode;

  auto fileNode{std::make_unique<FileNode>(rec, uncompressedSize, this)};
  const auto &node{mChildren.emplace_back(std::move(fileNode))};
  return static_cast<FileNode *>(node.get());
}

std::vector<bsa::Node *> bsa::FolderNode::getChildren() const {
  std::vector<bsa::Node *> out(mChildren.size(), nullptr);
  std::transform(mChildren.begin(), mChildren.end(), out.begin(),
                 [](const auto &ptr) { return ptr.get(); });
  return out;
}

bsa::BsaContext::BsaContext(std::string archiveName)
    : mBsaReader(std::move(archiveName)),
      mRoot(std::make_unique<bsa::FolderNode>("/", nullptr)) {

  for (bsa::FolderView folder : mBsaReader) {
    // TODO: oo::Path(std::string_view) constructor
    const oo::Path folderPath{std::string(folder.name())};

    bsa::FolderNode *folderNode{getRoot()};

    for (std::string_view components{folderPath.view()};;) {
      // Get the oldest directory, i.e everything up to the first '/'
      // By construction components does not begin or end with a '/'
      const auto pos{components.find('/')};
      std::string first{components.substr(0, pos)};
      folderNode = folderNode->addChildFolder(std::move(first));

      // Break if we've run out of directory components, otherwise string up to
      // and including the first '/'
      if (pos == std::string_view::npos) break;
      components = components.substr(pos + 1);
    }

    // `folderNode` now points to correct folder in the tree, so add the files
    for (bsa::FileView file : folder) {
      bsa::FileView fileRec{mBsaReader.getRecord(folder.hash(), file.hash())};
      if (fileRec.compressed()) {
        uint32_t size{mBsaReader.uncompressedSize(folder.hash(), file.hash())};
        folderNode->addChildFile(fileRec, size);
      } else {
        folderNode->addChildFile(fileRec);
      }
    }
  }
}

bsa::FolderNode *bsa::BsaContext::findFolder(std::string foldername) const {
  FolderNode *folderNode{getRoot()};
  if (foldername == folderNode->getName()) return folderNode;

  oo::Path path{std::move(foldername)};

  for (std::string_view components{path.view()};;) {
    const auto pos{components.find('/')};
    std::string first{components.substr(0, pos)};
    folderNode = folderNode->findChildFolder(first);
    if (!folderNode || pos == std::string_view::npos) break;

    components = components.substr(pos + 1);
  }

  return folderNode;
}

bsa::Node *bsa::BsaContext::findEntry(std::string filename) const {
  FolderNode *rootNode{getRoot()};
  if (filename == rootNode->getName()) return rootNode;

  FolderNode *folderNode{findFolder(filename)};
  if (folderNode) return folderNode;

  auto &&[base, last]{bsa::splitPath(std::move(filename))};

  FolderNode *baseNode{findFolder(base)};
  return baseNode ? baseNode->findChildFile(last) : nullptr;
}

int bsa::BsaContext::open(std::string folder, std::string file) {
  BsaHashPair hashPair(std::move(folder), std::move(file));
  try {
    mOpenFiles.try_emplace(hashPair,
                           getReader().stream(hashPair.first, hashPair.second));
  } catch (const std::exception &e) {
    return -ENOENT;
  }

  return 0;
}

int bsa::BsaContext::close(std::string folder, std::string file) {
  BsaHashPair hashPair(std::move(folder), std::move(file));
  mOpenFiles.erase(hashPair);

  return 0;
}

bool bsa::BsaContext::isOpen(std::string folder, std::string file) {
  BsaHashPair hashPair(std::move(folder), std::move(file));
  //C++20: return mOpenFiles.contains(hashPair);
  return mOpenFiles.count(hashPair) > 0;
}

std::istream &bsa::BsaContext::getStream(std::string folder, std::string file) {
  BsaHashPair hashPair(std::move(folder), std::move(file));
  return mOpenFiles.at(hashPair);
}

std::pair<std::string, std::string> bsa::splitPath(const char *path) {
  return bsa::splitPath(std::string(path));
}

std::pair<std::string, std::string> bsa::splitPath(std::string &&path) {
  const oo::Path fullPath{std::move(path)};
  return std::make_pair(std::string{fullPath.folder()},
                        std::string{fullPath.filename()});
}

bsa::BsaContext &bsa::getBsaContext(std::optional<std::string> filename) {
  static bsa::BsaContext ctx = [&]() {
    if (filename) {
      return bsa::BsaContext(std::move(*filename));
    } else {
      throw std::runtime_error("No archive given on first call to"
                               "bsa::getBsaContext()");
    }
  }();
  return ctx;
}
