#include "fuse.hpp"
#include "fuse_operations.hpp"
#include "fuse_options.hpp"

int main(int argc, char *argv[]) {
  fuser::Args args{argc, argv, 0};
  bsa::CmdOptConf conf{};

  fuser::optParse(&args, &conf, bsa::fuseCmdOpts.data(), bsa::handleCmdOpts);

  return fuser::main(args.argc, args.argv, bsa::fuseOps);
}