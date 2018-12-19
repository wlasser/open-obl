#define _FILE_OFFSET_BITS 64
#define FUSE_USE_VERSION 29
#include <fuse.h>

namespace posix {
using stat = struct ::stat;
} // namespace posix

namespace bsa {

int getattr(const char *path, posix::stat *stbuf) {
  *stbuf = {};

  return 0;
}

} // namespace bsa

int main(int argc, char *argv[]) {
  return 0;
}