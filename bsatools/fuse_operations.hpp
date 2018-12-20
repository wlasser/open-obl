#ifndef OPENOBLIVION_BSATOOLS_FUSE_OPERATIONS_HPP
#define OPENOBLIVION_BSATOOLS_FUSE_OPERATIONS_HPP

#include "fuse.hpp"

namespace bsa {

int getAttr(const char *path, posix::stat *stbuf);

/// Filesystem operations
constexpr inline fuser::Operations fuseOps = []() {
  fuser::Operations ops{};
  ops.getattr = &bsa::getAttr;
  return ops;
}();

} // namespace bsa

#endif // OPENOBLIVION_BSATOOLS_FUSE_OPERATIONS_HPP
