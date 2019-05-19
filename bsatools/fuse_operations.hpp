#ifndef OPENOBLIVION_BSATOOLS_FUSE_OPERATIONS_HPP
#define OPENOBLIVION_BSATOOLS_FUSE_OPERATIONS_HPP

#include "fuse.hpp"

namespace bsa {

/// \ingroup OpenOblivionBsaFuse
int getAttr(const char *path, Posix::stat *stbuf);

/// \ingroup OpenOblivionBsaFuse
int readDir(const char *path, void *buf, fuser::FillDirFun fillerFun,
            Posix::off_t offset, fuser::FileInfo *info);

/// \ingroup OpenOblivionBsaFuse
int open(const char *path, fuser::FileInfo *info);

/// \ingroup OpenOblivionBsaFuse
int read(const char *path, char *buf, std::size_t size, Posix::off_t offset,
         fuser::FileInfo *info);

/// \ingroup OpenOblivionBsaFuse
int release(const char *path, fuser::FileInfo *info);

/// Filesystem operations
/// \ingroup OpenOblivionBsaFuse
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
