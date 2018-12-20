#define _FILE_OFFSET_BITS 64
#define FUSE_USE_VERSION 29
#include <fuse.h>
#include <cstddef>
#include <cstdlib>
#include <iostream>

namespace posix {

using stat = struct ::stat;

} // namespace posix

namespace fuser {

using Operations = struct ::fuse_operations;
using OptProc = ::fuse_opt_proc_t;

using Args = struct ::fuse_args;
using Opt = struct ::fuse_opt;

int main(int argc, char *argv[], const fuser::Operations &ops) {
  return ::fuse_main(argc, argv, &ops, nullptr);
}

int optAddArg(Args *args, const char *arg) {
  return ::fuse_opt_add_arg(args, arg);
}

int optParse(Args *args, void *data, const Opt opts[], OptProc proc) {
  return ::fuse_opt_parse(args, data, opts, proc);
}

template<class T, class = std::enable_if_t<
    std::is_enum_v<T> && std::is_same_v<std::underlying_type_t<T>, int>>>
constexpr Opt makeOptKey(const char *templ, T key) {
  return FUSE_OPT_KEY(templ, static_cast<int>(key));
}

constexpr Opt makeOptEnd() {
  return FUSE_OPT_END;
}

} // namespace fuser

namespace bsa {

int getattr(const char *path, posix::stat *stbuf) {
  *stbuf = {};

  return 0;
}

constexpr static inline fuser::Operations FuseOps = []() {
  fuser::Operations ops{};
  ops.getattr = &bsa::getattr;
  return ops;
}();

struct CmdConf {
  char *archivePath{};
};

enum class CmdKeyOpt : int {
  Help, Version
};

constexpr static inline fuser::Opt FuseCmdOpts[] = {
    fuser::Opt{"archive=%s", offsetof(CmdConf, archivePath), 0},
    fuser::makeOptKey("-V", CmdKeyOpt::Version),
    fuser::makeOptKey("--version", CmdKeyOpt::Version),
    fuser::makeOptKey("-h", CmdKeyOpt::Help),
    fuser::makeOptKey("--help", CmdKeyOpt::Help),
    fuser::makeOptEnd()
};

int handleCmdOpts(void *data, const char *arg, int key, fuser::Args *outArgs) {
  switch (CmdKeyOpt{key}) {
    case CmdKeyOpt::Help: {
      std::cerr << "usage: " << outArgs->argv[0] << "mountpoint [options]\n"
                << R"cmd(
general options:
  -o opt,[opt...]   mount options
  -h, --help        display this help and exit
  -V, --version     display version information and exit

mount options:
  -o archive=STRING path of BSA archive to open

)cmd";
      fuser::main(outArgs->argc, outArgs->argv, bsa::FuseOps);
      std::exit(1);
    }
    case CmdKeyOpt::Version: {
      std::cerr << outArgs->argv[0] << " prerelease\n";
      fuser::optAddArg(outArgs, "--version");
      fuser::main(outArgs->argc, outArgs->argv, bsa::FuseOps);
      std::exit(0);
    }
  }

  // Keep the argument
  return 1;
}

} // namespace bsa

int main(int argc, char *argv[]) {
  fuser::Args args{argc, argv, 0};
  bsa::CmdConf conf{};

  fuser::optParse(&args, &conf, bsa::FuseCmdOpts, bsa::handleCmdOpts);

  return fuser::main(args.argc, args.argv, bsa::FuseOps);
}