#ifndef OPENOBLIVION_BSATOOLS_FUSE_OPTIONS_HPP
#define OPENOBLIVION_BSATOOLS_FUSE_OPTIONS_HPP

#include <array>
#include <cstddef>

namespace bsa {

/// Configuration determined by command-line arguments.
/// \ingroup OpenOblivionBsaFuse
struct CmdOptConf {
  /// Filepath of the Bsa archive to open
  char *archivePath{};
};

/// Key command-line options.
/// \ingroup OpenOblivionBsaFuse
enum class CmdOptKey : int {
  Help, Version
};

/// Command-line options
/// \ingroup OpenOblivionBsaFuse
constexpr inline std::array<fuser::Opt, 6> fuseCmdOpts{
    fuser::Opt{"archive=%s", offsetof(CmdOptConf, archivePath), 0},
    fuser::makeOptKey("-V", CmdOptKey::Version),
    fuser::makeOptKey("--version", CmdOptKey::Version),
    fuser::makeOptKey("-h", CmdOptKey::Help),
    fuser::makeOptKey("--help", CmdOptKey::Help),
    fuser::makeOptEnd()
};

/// \ingroup OpenOblivionBsaFuse
int handleCmdOpts(void *data, const char *arg, int key, fuser::Args *outArgs);

} // namespace bsa

#endif // OPENOBLIVION_BSATOOLS_FUSE_OPTIONS_HPP
