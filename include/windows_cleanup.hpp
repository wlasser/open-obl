/// \file windows_cleanup.hpp
/// `windows.h` poisons everything that touches it with unconditionally defined
/// macros with common names. This header attempts to clean up the mess by
/// undefining any such macros that aren't needed.
///
/// This file should be included at the end of the include list of any file
/// that includes any other header that touches `windows.h`, but does not
/// include this file. That includes anything that might need platform-dependent
/// behaviour, such as OGRE or spdlog.
///
/// If you find that you need some common word that `windows.h` has stolen for
/// its own purposes, feel free to undefine it for everyone here. In the
/// unlikely event that you need one of those macros that `windows.h` has
/// defined, please consider if the same can be achieved in a
/// platform-independent way. If not, please redefine the macro yourself then
/// undefine it afterwards; the stability of `windows.h` (i.e. the whole reason
/// we're in this mess) will hopefully ensure such a redefinition will not
/// become incorrect in the future.

#undef near
#undef far

#undef min
#undef max

#undef LoadMenu
#undef GetCurrentTime
