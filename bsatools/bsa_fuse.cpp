#include "bsa_fuse.hpp"
#include "fuse.hpp"
#include "fuse_operations.hpp"
#include "fuse_options.hpp"
#include "fs/path.hpp"
#include <algorithm>
#include <iostream>

int main(int argc, char *argv[]) {
  fuser::Args args{argc, argv, 0};
  bsa::CmdOptConf conf{};

  fuser::optParse(&args, &conf, bsa::fuseCmdOpts.data(), bsa::handleCmdOpts);

  if (!conf.archivePath) {
    std::cerr << "please provide an archive parameter\n";
    std::exit(1);
  }

  // Construct the global bsa context
  bsa::getBsaContext(conf.archivePath);

  return fuser::main(args.argc, args.argv, bsa::fuseOps);
}

bsa::FolderNode *bsa::FolderNode::findChildFolder(std::string name) {
  auto it{std::find_if(
      mChildren.begin(), mChildren.end(), [&name](const auto &child) {
        return child->isFolder() && child->getName() == name;
      })};
  if (it != mChildren.end()) return static_cast<bsa::FolderNode *>(it->get());
  return nullptr;
}

bsa::FileNode *bsa::FolderNode::findChildFile(std::string name) {
  auto it{std::find_if(
      mChildren.begin(), mChildren.end(), [&name](const auto &child) {
        return !child->isFolder() && child->getName() == name;
      })};
  if (it != mChildren.end()) return static_cast<bsa::FileNode *>(it->get());
  return nullptr;
}

bsa::FolderNode *bsa::FolderNode::addChildFolder(std::string name) {
  if (auto *childNode{findChildFolder(name)}) return childNode;

  const auto &node
      {mChildren.emplace_back(std::make_unique<bsa::FolderNode>(name, this))};
  return static_cast<bsa::FolderNode *>(node.get());
}

bsa::FileNode *
bsa::FolderNode::addChildFile(const bsa::BsaReader::FileRecord &rec) {
  if (auto *childNode{findChildFile(rec.name)}) return childNode;

  const auto &node
      {mChildren.emplace_back(std::make_unique<bsa::FileNode>(rec, this))};
  return static_cast<bsa::FileNode *>(node.get());
}

bsa::BsaContext::BsaContext(std::string filename)
    : mBsaReader(std::move(filename)),
      mRoot(std::make_unique<bsa::FolderNode>("/", nullptr)) {

  for (const auto &folderRec : mBsaReader) {
    // Precompute hash for faster lookup in files
    const uint64_t folderHash{bsa::genHash(folderRec.name,
                                           bsa::HashType::Folder)};

    // By construction components does not begin or end with a '/'
    const oo::Path folderPath{folderRec.name};
    std::string_view components{folderPath.view()};

    bsa::FolderNode *folderNode{getRoot()};

    for (;;) {
      // Get the oldest directory, i.e everything up to the first '/'
      const auto pos{components.find('/')};
      const std::string first{components.substr(0, pos)};

      folderNode = folderNode->addChildFolder(first);

      // Break if we've run out of directory components, otherwise strip `first`
      // and the trailing '/'
      if (pos == std::string_view::npos) break;
      components = components.substr(pos + 1);
    }

    // `folderNode` now points to correct folder in the tree, so add the files
    for (const auto &filename : folderRec.files) {
      const uint64_t fileHash{bsa::genHash(filename, bsa::HashType::File)};
      const auto fileRec{mBsaReader.getRecord(folderHash, fileHash)};
      if (!fileRec) {
        // TODO: Warn that the file could not be found
      }
      folderNode->addChildFile(*fileRec);
    }
  }
}

const bsa::BsaContext &bsa::getBsaContext(std::optional<std::string> filename) {
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
