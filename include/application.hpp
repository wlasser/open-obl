#ifndef OPENOBLIVION_APPLICATION_HPP
#define OPENOBLIVION_APPLICATION_HPP

#include "application_context.hpp"
#include "controls.hpp"
#include "fs/path.hpp"
#include "modes/console_mode.hpp"
#include "modes/game_mode.hpp"
#include "modes/menu_mode.hpp"
#include "sdl/sdl.hpp"
#include <Ogre.h>
#include <string>
#include <vector>

using ModeVariant = std::variant<GameMode, ConsoleMode, MenuMode>;

class Application : public Ogre::FrameListener {
 private:
  Ogre::LogManager ogreLogMgr{};
  std::unique_ptr<Ogre::LogListener> ogreLogListener{};

  ApplicationContext ctx{};
  std::vector<ModeVariant> modeStack{};

  /// Set up the logger.
  /// Ogre's logging facilities are pretty good but fall down when it comes to
  /// formatting. Using boost::format gets pretty tedious so we use spdlog,
  /// which has the fmt library built in. Obviously we still want Ogre's
  /// internal log messages though, so we use a LogListener to intercept the
  /// standard Ogre log messages and hand them over to spdlog.
  void createLoggers();

  /// Read the default and user ini files and store them in GameSettings.
  /// \pre createLoggers() has been called. In particular,
  /// `spdlog::get(settings::log) != nullptr`.
  static void loadIniConfiguration();
  //C++20: [[expects: spdlog::get(oo::LOG) != nullptr]];

  /// Set the Ogre render system.
  /// systemName must be one of
  /// - `OpenGL 3+ Rendering Subsystem`
  /// - `OpenGL Rendering Subsystem`
  ///
  /// Currently only `OpenGL 3+ Rendering Subsystem` is supported, there may be
  /// graphical errors when using other render systems.
  /// \exception std::runtime_error Thrown if the render system is not found by
  ///   Ogre::Root::getRenderSystemByName.
  static void setRenderSystem(Ogre::Root *root, const std::string &systemName);

  /// Initialize Ogre, call `setRenderSystem`, and initialize the
  /// `Ogre::OverlaySystem`.
  /// This is a member function as it registers `this` as an
  /// `Ogre::FrameListener`.
  std::tuple<std::unique_ptr<Ogre::Root>, std::unique_ptr<Ogre::OverlaySystem>>
  createOgreRoot();

  /// Construct an SDL window and embed an Ogre::RenderWindow inside.
  /// The window is created with width `Display.iSize W` and height
  /// `Display.iSize H`, and is fullscreen iff `Display.bFull Screen` is true.
  /// \exception std::runtime_error Thrown if at least one of `Display.iSize W`
  ///   and `Display.iSize H` are non-positive.
  std::tuple<sdl::WindowPtr, Ogre::RenderWindowPtr>
  static createWindow(const std::string &windowName);

  /// Prepend the `masterPath` to each filename in the comma-separated `list`,
  /// returning those that exist.
  /// \param masterPath The path the bsa files are in relative to the root,
  ///   usually `"Data"`
  /// \param list A comma and optionally whitespace-separated list of filenames,
  ///   such as `"Oblivion - Meshes.bsa, Oblivion - Sounds.bsa"`.
  ///   Trailing whitespace within each filename is ignored.
  static std::vector<oo::Path>
  parseBsaList(const oo::Path &masterPath, const std::string &list);

  /// Detect the resource type of path and declare it with the correct
  /// Ogre::ManualResourceLoader, if any.
  void declareResource(const oo::Path &path, const std::string &resourceGroup);

  /// Add the given bsa archive as a resource location.
  void declareBsaArchive(const oo::Path &bsaFilename);

  /// Declare all the resources in the given bsa archive.
  void declareBsaResources(const oo::Path &bsaFilename);

  /// Declare all the resources in the given folder.
  /// This should be called *after* `declareBsaResources`, as it will not
  /// declare a resource if has already been declared, unlike
  /// `declareBsaResources` which doesn't bother checking.
  ///
  // There are two reasons for this method; the first is that because the
  // filesystem is case-sensitive, Ogre will not have properly indexed any files
  // whose names are not already normalized, and thus they will not be
  // openable, unless they already exist in a Bsa file, in which the Bsa entry
  // will be chosen over the filesystem; the second is that while Ogre will
  // index any files whose name is already normalized, they will not have been
  // declared so will not be openable in any non-generic situation (i.e. opening
  // a TextResource not a Resource).
  void declareFilesystemResources(const oo::Path &foldername);

  /// Return all esm files in the `masterPath` sorted by decreasing modification
  /// date, followed by all esp files in the `masterPath` sorted by decreasing
  /// modification date.
  static std::vector<oo::Path> getLoadOrder(const oo::Path &masterPath);

  /// Poll for SDL events and process all that have occurred.
  void pollEvents();

  /// Pop the top mode off the stack, if any.
  void popMode() {
    if (!modeStack.empty()) modeStack.pop_back();
  }

  /// Push a mode onto the stack and call its enter method.
  /// \tparam Ts Should model the Mode concept, see GameMode and ConsoleMode.
  template<class ...Ts>
  void pushMode(std::variant<Ts...> mode) {
    // First visit moves into a new variant so need a second visit.
    std::visit([this](auto &&state) {
      modeStack.emplace_back(std::move(state));
    }, mode);
    std::visit([this](auto &&state) { state.enter(ctx); }, modeStack.back());
  }

  /// Refocus the top mode on the stack, if any.
  void refocusMode() {
    if (modeStack.empty()) return;
    std::visit([this](auto &&state) { state.refocus(ctx); }, modeStack.back());
  }

  /// Return true iff `e` is an sdl::EventType::KeyUp, sdl::EventType::KeyDown,
  /// sdl::EventType::TextInput, or sdl::EventType::TextEditing event.
  static bool isKeyboardEvent(const sdl::Event &e) noexcept;

  /// Return true iff `e` is an sdl::EventType::MouseMotion,
  /// sdl::EventType::MouseWheel, sdl::EventType::MouseButtonUp, or
  /// sdl::EventType::MouseButtonDown event.
  static bool isMouseEvent(const sdl::Event &e) noexcept;

 public:
  explicit Application(std::string windowName);

  Ogre::Root *getRoot() {
    return ctx.ogreRoot.get();
  }

  bool frameStarted(const Ogre::FrameEvent &event) override;
  bool frameRenderingQueued(const Ogre::FrameEvent &event) override;
  bool frameEnded(const Ogre::FrameEvent &event) override;
};

#endif // OPENOBLIVION_APPLICATION_HPP
