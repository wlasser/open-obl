#ifndef OPENOBLIVION_APPLICATION_HPP
#define OPENOBLIVION_APPLICATION_HPP

#include "bullet/configuration.hpp"
#include "bullet/collision.hpp"
#include "bsa_archive_factory.hpp"
#include "character_controller/player_controller.hpp"
#include "controls.hpp"
#include "gui/gui.hpp"
#include "nifloader/mesh_loader.hpp"
#include "nifloader/collision_object_loader.hpp"
#include "esp_coordinator.hpp"
#include "fs/path.hpp"
#include "meta.hpp"
#include "ogrebullet/debug_drawer.hpp"
#include "ogrebullet/collision_object_manager.hpp"
#include "ogrebullet/rigid_body.hpp"
#include "ogreimgui/imgui_manager.hpp"
#include "ogre/text_resource_manager.hpp"
#include "ogre/window.hpp"
#include "resolvers/interior_cell_resolver.hpp"
#include "resolvers/door_resolver.hpp"
#include "resolvers/light_resolver.hpp"
#include "resolvers/static_resolver.hpp"
#include "sdl/sdl.hpp"
#include <boost/format.hpp>
#include <Ogre.h>
#include <SDL2/SDL.h>
#include <spdlog/spdlog.h>
#include <memory>
#include <string>

class Application : public Ogre::FrameListener {
 private:
  std::unique_ptr<Ogre::BsaArchiveFactory> bsaArchiveFactory{};
  std::unique_ptr<Ogre::RigidBodyFactory> rigidBodyFactory{};

  std::shared_ptr<spdlog::logger> logger{};
  std::unique_ptr<Ogre::LogManager> ogreLogMgr{};
  std::unique_ptr<Ogre::LogListener> ogreLogListener{};

  std::unique_ptr<Ogre::Root> ogreRoot{};
  sdl::Init sdlInit;

  sdl::WindowPtr sdlWindow{nullptr, nullptr};
  Ogre::RenderWindowPtr ogreWindow;

  std::unique_ptr<KeyMap> keyMap{};

  std::unique_ptr<bullet::Configuration> bulletConf{};

  nifloader::MeshLoader nifLoader{};
  nifloader::CollisionObjectLoader nifCollisionLoader{};

  std::unique_ptr<Ogre::CollisionObjectManager> collisionObjectMgr{};
  std::unique_ptr<Ogre::TextResourceManager> textResourceMgr{};

  std::unique_ptr<gui::LoadingMenu> menuLoadingMenu{};

  std::unique_ptr<esp::EspCoordinator> espCoordinator{};

  std::unique_ptr<DoorResolver> doorRes{};
  std::unique_ptr<LightResolver> lightRes{};
  std::unique_ptr<StaticResolver> staticRes{};
  std::unique_ptr<InteriorCellResolver> interiorCellRes{};

  std::shared_ptr<InteriorCell> currentCell{};
  bullet::CollisionCaller collisionCaller{};
  std::unique_ptr<character::PlayerController> playerController{};

  bool drawBulletDebug{false};
  std::unique_ptr<Ogre::DebugDrawer> debugDrawer{};
  std::unique_ptr<Ogre::ImGuiManager> imguiMgr{};

  /// Set up the logger.
  /// Ogre's logging facilities are pretty good but fall down when it comes to
  /// formatting. Using boost::format gets pretty tedious so we use spdlog,
  /// which has the fmt library built in. Obviously we still want Ogre's
  /// internal log messages though, so we use a LogListener to intercept the
  /// standard Ogre log messages and hand them over to spdlog.
  void createLoggers();

  /// Read the default and user ini files and store them in GameSettings.
  void loadIniConfiguration();

  /// Set the Ogre render system.
  /// systemName must be one of
  /// - `OpenGL 3+ Rendering Subsystem`
  /// - `OpenGL Rendering Subsystem`
  ///
  /// Currently only `OpenGL 3+ Rendering Subsystem` is supported, there may be
  /// graphical errors when using other render systems.
  /// \exception std::runtime_error Thrown if the render system is not found by
  ///   Ogre::Root::getRenderSystemByName.
  void setRenderSystem(const std::string &systemName);

  /// Construct an SDL window and embed an Ogre::RenderWindow inside.
  /// The window is created with width `Display.iSize W` and height
  /// `Display.iSize H`, and is fullscreen iff `Display.bFull Screen` is true.
  /// \exception std::runtime_error Thrown if at least one of `Display.iSize W`
  ///   and `Display.iSize H` are non-positive.
  void createWindow(const std::string &windowName);

  /// Prepend the `masterPath` to each filename in the comma-separated `list`,
  /// returning those that exist.
  /// \param masterPath The path the bsa files are in relative to the root,
  ///   usually `"Data"`
  /// \param list A comma and optionally whitespace-separated list of filenames,
  ///   such as `"Oblivion - Meshes.bsa, Oblivion - Sounds.bsa"`.
  ///   Trailing whitespace within each filename is ignored.
  std::vector<fs::Path> parseBsaList(const fs::Path &masterPath,
                                     const std::string &list);

  /// Detect the resource type of path and declare it with the correct
  /// Ogre::ManualResourceLoader, if any.
  void declareResource(const fs::Path &path, const std::string &resourceGroup);

  /// Add the given bsa archive as a resource location.
  void declareBsaArchive(const fs::Path &bsaFilename);

  /// Declare all the resources in the given bsa archive.
  void declareBsaResources(const fs::Path &bsaFilename);

  /// Return all esm files in the `masterPath` sorted by decreasing modification
  /// date, followed by all esp files in the `masterPath` sorted by decreasing
  /// modification date.
  std::vector<fs::Path> getLoadOrder(const fs::Path &masterPath);

  /// Poll for SDL events and process all that have occured.
  void pollEvents();

  /// Run all registered collision callbacks with the collisions for this frame.
  void dispatchCollisions();

  /// Return a reference to the object under the crosshair.
  /// This works by raytesting against all collision objects in the current
  /// cell within `iActivatePickLength` units. If no object is found, return the
  /// null reference `0`.
  RefId getCrosshairRef();

  /// Turn the display of collision wireframes on or off.
  void enableBulletDebugDraw(bool enable);

  /// Return true iff `e` is an sdl::EventType::KeyUp, sdl::EventType::KeyDown,
  /// sdl::EventType::TextInput, or sdl::EventType::TextEditing event.
  bool isKeyboardEvent(const sdl::Event &e) const noexcept;

  /// Return true iff `e` is an sdl::EventType::MouseMotion,
  /// sdl::EventType::MouseWheel, sdl::EventType::MouseButtonUp, or
  /// sdl::EventType::MouseButtonDown event.
  bool isMouseEvent(const sdl::Event &e) const noexcept;

 public:
  explicit Application(std::string windowName);

  Ogre::Root *getRoot() {
    return ogreRoot.get();
  }

  bool frameStarted(const Ogre::FrameEvent &event) override;
  bool frameRenderingQueued(const Ogre::FrameEvent &event) override;
  bool frameEnded(const Ogre::FrameEvent &event) override;
};

#endif // OPENOBLIVION_APPLICATION_HPP
