#ifndef OPENOBLIVION_APPLICATION_HPP
#define OPENOBLIVION_APPLICATION_HPP

#include "application_context.hpp"
#include "controls.hpp"
#include "fs/path.hpp"
#include "modes/console_mode.hpp"
#include "modes/game_mode.hpp"
#include "modes/load_menu_mode.hpp"
#include "modes/loading_menu_mode.hpp"
#include "modes/main_menu_mode.hpp"
#include "sdl/sdl.hpp"
#include <Ogre.h>
#include <RenderSystems/GL3Plus/OgreGL3PlusPlugin.h>
#include <optional>
#include <string>
#include <vector>

namespace oo {

/// All possible `Mode`s that the oo::Application can be in.
/// \ingroup OpenOblivionModess
using ModeVariant = std::variant<oo::GameMode,
                                 oo::ConsoleMode,
                                 oo::LoadMenuMode,
                                 oo::LoadingMenuMode,
                                 oo::MainMenuMode>;

class Application : public Ogre::FrameListener {
 private:
  Ogre::LogManager ogreLogMgr{};
  std::unique_ptr<Ogre::LogListener> ogreLogListener{};

  ApplicationContext ctx{};
  std::vector<oo::ModeVariant> modeStack{};

  /// \name Dummy Scene Manager
  /// This machinery is necessary to display text on the opening menu screen,
  /// for reasons discussed below.
  ///
  /// In order to render text correctly, each Ogre::TextAreaOverlayElement asks
  /// the Ogre::OverlayManager for the dimensions of the viewport currently
  /// being rendered to. Until a viewport is created *and rendered to using the
  /// Ogre::OverlaySystem*, the returned dimensions are `(0, 0)`. This means
  /// that text created before the first frame of the application will have zero
  /// width and not be rendered.
  ///
  /// This is especially awkward when the first mode is a MenuMode, since then
  /// the Ogre::Overlay is created before the MenuMode's own Ogre::SceneManager.
  /// Since we can't create a camera and add a viewport without a scene manager,
  /// the Ogre::OverlayManager must be primed with the correct screen dimensions
  /// before we ever start rendering a mode.
  ///
  /// To achieve this, `createDummySceneManager()` creates an Ogre::SceneManager
  /// whose only purpose is to contain an Ogre::Camera used to set the viewport
  /// size. `createDummyRenderQueue()` then forces the Ogre::OverlayManager to
  /// update its cached viewport dimensions to the correct ones by pretending to
  /// render all the overlays---of which there aren't any, yet---but doing so
  /// outside of Ogre's render loop.
  /// @{
  void createDummySceneManager();
  void createDummyRenderQueue();

  Ogre::SceneManager *dummyScnMgr{};
  Ogre::Camera *dummyCamera{};
  ///@}

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
  std::tuple<std::unique_ptr<Ogre::Root>,
             std::unique_ptr<Ogre::OverlaySystem>,
             std::unique_ptr<Ogre::GL3PlusPlugin>>
  createOgreRoot();

  /// Construct an SDL window and embed an Ogre::RenderWindow inside.
  /// The window is created with width `Display.iSize W` and height
  /// `Display.iSize H`, and is fullscreen iff `Display.bFull Screen` is true.
  /// \exception std::runtime_error Thrown if at least one of `Display.iSize W`
  ///   and `Display.iSize H` are non-positive.
  std::tuple<sdl::WindowPtr, Ogre::RenderWindowPtr>
  static createWindow(const std::string &windowName);

  /// Set the global terrain options.
  void setTerrainOptions();

  /// Set the audio settings from the ini configuration.
  void setSoundSettings();

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
    if (!modeStack.empty()) {
      std::visit([this](auto &&newState, auto &oldState) {
        using NewStateType = std::decay_t<decltype(newState)>;
        using OldStateType = std::decay_t<decltype(oldState)>;
        if constexpr (std::is_base_of_v<MenuModeBase<OldStateType>,
                                        OldStateType>
            && HideOverlayOnTransition<NewStateType>::value) {
          oldState.hideOverlay();
        }
        modeStack.emplace_back(std::forward<decltype(newState)>(newState));
      }, std::move(mode), modeStack.back());
    } else {
      std::visit([this](auto &&state) {
        modeStack.emplace_back(std::forward<decltype(state)>(state));
      }, std::move(mode));
    }
    // First visit moves into a new variant so need a second visit.
    std::visit([this](auto &&state) { state.enter(ctx); }, modeStack.back());
  }

  /// Refocus the top mode on the stack, if any.
  void refocusMode() {
    if (modeStack.empty()) return;
    std::visit([this](auto &&state) { state.refocus(ctx); }, modeStack.back());
  }

  /// Store the new mode to be inserted here, if a mode transition is required
  /// outside of the usual points (such as in a console command).
  /// The mode stack should not be modified during the execution of a function
  /// belonging to a mode, as immediately after the modification `this` is
  /// invalidated. This should be assumed to happen even if a new mode is
  /// pushed onto the stack, since the stack might grow and reallocate.
  std::optional<ModeVariant> deferredMode{};

  /// Register all the console commands with the console engine.
  void registerConsoleFunctions();

  /// Register all scripting commands with the scripting engine.
  void registerScriptFunctions();

 public:
  explicit Application(std::string windowName);

  Ogre::Root *getRoot() {
    return ctx.ogreRoot.get();
  }

  void quit();

  bool frameStarted(const Ogre::FrameEvent &event) override;
  bool frameRenderingQueued(const Ogre::FrameEvent &event) override;
  bool frameEnded(const Ogre::FrameEvent &event) override;

  /// Whether the game is currently running in GameMode.
  bool isGameMode() const;

  /// Whether the game is currently running in ConsoleMode.
  bool isConsoleMode() const;

  /// Return a reference to the current GameMode state.
  /// \pre `isMenuMode() == true`
  oo::GameMode &getGameMode() /*C++20:[[expects: isGameMode()]]*/;

  /// Whether a GameMode state is present somewhere in the mode stack.
  bool isGameModeInStack() const;

  /// Return a reference to the GameMode state closest to the top of the mode
  /// stack.
  /// \pre `isGameModeInStack() == true`
  oo::GameMode &getGameModeInStack() /*C++20:[[expects: isGameModeInStack()]]*/;

  /// Open a new menu at the next available opportunity, adding it to the top of
  /// the mode stack. If the currently running state is a ConsoleMode, then pop
  /// that state first.
  template<gui::MenuType Type> void openMenu();
};

inline Application *getApplication(std::optional<Application *> ptr = {}) {
  static Application *mPtr{*ptr};
  return mPtr;
}

template<gui::MenuType Type> void Application::openMenu() {
  deferredMode.emplace(std::in_place_type<oo::MenuMode<Type>>, ctx);
}
} // namespace oo

#endif // OPENOBLIVION_APPLICATION_HPP
