#include "fuse_operations.hpp"

int bsa::getAttr(const char *path, posix::stat *stbuf) {
  *stbuf = {};
  return 0;
}
