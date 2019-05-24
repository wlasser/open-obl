#ifndef OPENOBL_BSATOOLS_FUSE_OPERATIONS_HPP
#define OPENOBL_BSATOOLS_FUSE_OPERATIONS_HPP

#include "fuse.hpp"

namespace bsa {

/// \ingroup OpenOBLBsaFuse
int getAttr(const char *path, Posix::stat *stbuf);

/// \ingroup OpenOBLBsaFuse
int readDir(const char *path, void *buf, fuser::FillDirFun fillerFun,
            Posix::off_t offset, fuser::FileInfo *info);

/// \ingroup OpenOBLBsaFuse
int open(const char *path, fuser::FileInfo *info);

/// \ingroup OpenOBLBsaFuse
int read(const char *path, char *buf, std::size_t size, Posix::off_t offset,
         fuser::FileInfo *info);

/// \ingroup OpenOBLBsaFuse
int release(const char *path, fuser::FileInfo *info);

/// Filesystem operations
/// \ingroup OpenOBLBsaFuse
constexpr inline fuser::Operations fuseOps = []() noexcept {
  fuser::Operations ops{};
  ops.getattr = bsa::getAttr;
  ops.readdir = bsa::readDir;
  ops.open = bsa::open;
  ops.read = bsa::read;
  return ops;
}();

} // namespace bsa

#endif // OPENOBL_BSATOOLS_FUSE_OPERATIONS_HPP
