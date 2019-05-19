#include "fuse_operations.hpp"
#include "fuse_options.hpp"
#include <iostream>
#include <cstdlib>

namespace {

const char *helpText = R"cmd(
general options:
  -o opt,[opt...]   mount options
  -h, --help        display this help and exit
  -V, --version     display version information and exit

mount options:
  -o archive=STRING path of BSA archive to open
)cmd";

}

int bsa::handleCmdOpts(void */*data*/,
                       const char */*arg*/,
                       int key,
                       fuser::Args *outArgs) {
  switch (bsa::CmdOptKey{key}) {
    case bsa::CmdOptKey::Help: {
      std::cerr << "usage: " << outArgs->argv[0] << " mountpoint [options]\n"
                << helpText << '\n';
      fuser::optAddArg(outArgs, "--help");
      fuser::main(outArgs->argc, outArgs->argv, bsa::fuseOps);
      std::exit(1);
    }

    case CmdOptKey::Version: {
      std::cerr << outArgs->argv[0] << " prerelease\n";
      fuser::optAddArg(outArgs, "--version");
      fuser::main(outArgs->argc, outArgs->argv, bsa::fuseOps);
      std::exit(0);
    }
  }

  // Keep the argument
  return 1;
}
