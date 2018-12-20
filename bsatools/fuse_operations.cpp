#include "bsa_fuse.hpp"
#include "fuse_operations.hpp"
#include "fs/path.hpp"
#include <cerrno>

int bsa::getAttr(const char *path, posix::stat *stbuf) {
  // Clear stbuf
  *stbuf = {};

  const auto &bsaReader{bsa::getBsaContext().getReader()};

  const oo::Path fsPath{path};
  const std::string folder{fsPath.folder()};
  const std::string filename{fsPath.filename()};

  const auto recOpt{bsaReader.getRecord(folder, filename)};
  if (!recOpt) return -ENOENT;
  const auto fileRecord{*recOpt};

  stbuf->st_mode = S_IFREG | 0444;
  stbuf->st_nlink = 1;
  stbuf->st_size = fileRecord.size;

  return 0;
}
