#ifndef OPENOBL_APPLICATION_BASE_HPP
#define OPENOBL_APPLICATION_BASE_HPP

#include "ogre/window.hpp"
#include "sdl/sdl.hpp"
#include <OgreGL3PlusPlugin.h>
#include <OgreLog.h>

namespace oo {

/// Construct the main OGRE and OpenOBL loggers.
/// Both loggers write to the console and a log file with the given `logName`.
/// To spdlog, the OGRE logger is named `oo::OGRE_LOG` and the OpenOBL logger is
/// named `oo::LOG`.
///
/// For consistency, spdlog is used for OGRE's logger as well as our own by
/// using an `Ogre::LogListener` to intercept OGRE's log messages and hand them
/// over to spdlog. This functions constructs both loggers and hooks OGRE's up
/// to spdlog, returning the listener required to do so.
/// \returns A pointer to the listener that will intercept OGRE's log messages,
///          and whose lifetime must contain the lifetime of the `Ogre::Root`.
std::unique_ptr<Ogre::LogListener>
createLoggers(const std::string &filename, Ogre::LogManager &logMgr);

/// Set the OGRE render system by its name.
/// Currently only `OpenGL 3+ Rendering Subsystem` is supported; there may be
/// graphical errors when using other render systems, provided that they even
/// work at all.
/// \returns a pointer to the requested render system.
/// \throws std::runtime_error if the render system is not found by
///                            `Ogre::Root::getRenderSystemByName`.
Ogre::RenderSystem *
setRenderSystem(Ogre::Root *root, const std::string &systemName);

/// Load the GL3+ render system and set is as the current render system.
std::unique_ptr<Ogre::GL3PlusPlugin> startGl3Plus(Ogre::Root *root);

class Window {
  sdl::WindowPtr mSdlWin;
  Ogre::RenderWindowPtr mOgreWin;

 public:
  Window();
  Window(Ogre::Root *root, int width, int height, const std::string &name,
         sdl::WindowFlags flags);

  Ogre::RenderWindow *getOgreWindow() { return mOgreWin.get(); }
};

} // namespace oo

#endif // OPENOBL_APPLICATION_BASE_HPP
