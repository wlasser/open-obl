#include "bsa_fuse.hpp"
#include "fuse_operations.hpp"
#include "fs/path.hpp"
#include <algorithm>
#include <cerrno>

int bsa::getAttr(const char *path, posix::stat *stbuf) {
  // Clear stbuf
  *stbuf = {};

  Node *entry = [&]() -> Node * {
    if (path == std::string{"/"}) return bsa::getBsaContext().getRoot();
    else return bsa::getBsaContext().findEntry(path);
  }();
  if (!entry) return -ENOENT;

  if (entry->isFolder()) {
    auto *folderNode{static_cast<FolderNode *>(entry)};
    auto children{folderNode->getChildren()};
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 2 + std::count_if(children.begin(), children.end(),
                                        [](Node *node) {
                                          return node->isFolder();
                                        });
  } else {
    auto *fileNode{static_cast<FileNode *>(entry)};
    stbuf->st_mode = S_IFREG | 0444;
    stbuf->st_nlink = 1;
    stbuf->st_size = fileNode->getSize();
  }

  return 0;
}

int bsa::readDir(const char *path, void *buf, fuser::FillDirFun fillerFun,
                 posix::off_t /*offset*/, fuser::FileInfo * /*info*/) {
  FolderNode *folder{bsa::getBsaContext().findFolder(path)};
  if (!folder) return -ENOENT;

  fillerFun(buf, ".", nullptr, 0);
  fillerFun(buf, "..", nullptr, 0);
  for (Node *node : folder->getChildren()) {
    fillerFun(buf, node->getName().c_str(), nullptr, 0);
  }

  return 0;
}

int bsa::open(const char *path, fuser::FileInfo *info) {
  Node *entry{bsa::getBsaContext().findEntry(path)};
  if (!entry || entry->isFolder()) return -ENOENT;

  auto *fileNode{static_cast<FileNode *>(entry)};
  const oo::Path fullPath{path};
  std::string folder{fullPath.folder()};
  std::string filename{fullPath.filename()};

  auto &bsaContext{bsa::getBsaContext()};
  return bsaContext.open(std::move(folder), std::move(filename));
}

int bsa::read(const char *path, char *buf, std::size_t size,
              posix::off_t offset, fuser::FileInfo *info) {
  auto &bsaContext{bsa::getBsaContext()};

  const oo::Path fullPath{path};
  std::string folder{fullPath.folder()};
  std::string filename{fullPath.filename()};

  if (!bsaContext.isOpen(folder, filename)) return -EBADF;

  std::istream &is{bsaContext.getStream(std::move(folder),
                                        std::move(filename))};
  is.clear();
  is.seekg(offset);
  is.read(buf, size);
  return is.gcount();
}

int bsa::release(const char *path, fuser::FileInfo *info) {
  auto &bsaContext{bsa::getBsaContext()};

  const oo::Path fullPath{path};
  std::string folder{fullPath.folder()};
  std::string filename{fullPath.filename()};

  return bsaContext.close(std::move(folder), std::move(filename));
}
