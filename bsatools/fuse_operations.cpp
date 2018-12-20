#include "bsa_fuse.hpp"
#include "fuse_operations.hpp"
#include "fs/path.hpp"
#include <cerrno>

int bsa::getAttr(const char *path, posix::stat *stbuf) {
  // Clear stbuf
  *stbuf = {};

  if (path == std::string{"/"}) {
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 2;
    return 0;
  }

  Node *entry{bsa::getBsaContext().findEntry(path)};
  if (!entry) return -ENOENT;

  if (entry->isFolder()) {
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 2;
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
