#ifndef OPENOBLIVION_BSATOOLS_FUSE_OPERATIONS_HPP
#define OPENOBLIVION_BSATOOLS_FUSE_OPERATIONS_HPP

#include "fuse.hpp"

namespace bsa {

int getAttr(const char *path, posix::stat *stbuf);

int readDir(const char *path, void *buf, fuser::FillDirFun fillerFun,
            posix::off_t offset, fuser::FileInfo *info);

int open(const char *path, fuser::FileInfo *info);

int read(const char *path, char *buf, std::size_t size, posix::off_t offset,
         fuser::FileInfo *info);

int release(const char *path, fuser::FileInfo *info);

/// Filesystem operations
constexpr inline fuser::Operations fuseOps = []() {
  fuser::Operations ops{};
  ops.getattr = bsa::getAttr;
  ops.readdir = bsa::readDir;
  ops.open = bsa::open;
  ops.read = bsa::read;
  return ops;
}();

} // namespace bsa

#endif // OPENOBLIVION_BSATOOLS_FUSE_OPERATIONS_HPP
