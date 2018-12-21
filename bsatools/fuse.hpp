#ifndef OPENOBLIVION_BSATOOLS_FUSE_HPP
#define OPENOBLIVION_BSATOOLS_FUSE_HPP

#define _FILE_OFFSET_BITS 64
#define FUSE_USE_VERSION 29

#include <fuse.h>
#include <type_traits>

/// Very thin wrapper around some things we need in the POSIX C library.
/// The hope is that for example, `posix::stat` is much clearer than
/// `struct stat` when the reader has little familiarity with posix, and just as
/// clear for someone more experienced.
/// \ingroup OpenOblivionBsaFuse
namespace posix {

using stat = struct ::stat;
using off_t = ::off_t;

} // namespace posix

/// Wrappers around the C libfuse API.
/// If a function/struct etc. is needed from libfuse, then prefer writing and
/// using a thing wrapper here. The arguments for the `posix` namespace don't
/// really apply because libfuse is nicely namespaced already by the `fuse_`
/// prefix, mostly I'm just picky about aesthetics.
/// \ingroup OpenOblivionBsaFuse
namespace fuser {

using Operations = struct ::fuse_operations;
using OptProc = ::fuse_opt_proc_t;
using FillDirFun = ::fuse_fill_dir_t;
using FileInfo = struct ::fuse_file_info;

using Args = struct ::fuse_args;
using Opt = struct ::fuse_opt;

inline int main(int argc, char *argv[], const fuser::Operations &ops) {
  return ::fuse_main(argc, argv, &ops, nullptr);
}

inline int optAddArg(Args *args, const char *arg) {
  return ::fuse_opt_add_arg(args, arg);
}

inline int optParse(Args *args, void *data, const Opt opts[], OptProc proc) {
  return ::fuse_opt_parse(args, data, opts, proc);
}

template<class T, class = std::enable_if_t<
    std::is_enum_v<T> && std::is_same_v<std::underlying_type_t<T>, int>>>
constexpr Opt makeOptKey(const char *templ, T key) {
  return FUSE_OPT_KEY(templ, static_cast<int>(key));
}

inline constexpr Opt makeOptEnd() {
  return FUSE_OPT_END;
}

} // namespace fuser

#endif // OPENOBLIVION_BSATOOLS_FUSE_HPP
