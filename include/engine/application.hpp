#ifndef OPENOBLIVION_ENGINE_APPLICATION_HPP
#define OPENOBLIVION_ENGINE_APPLICATION_HPP

#include "bullet/configuration.hpp"
#include "bullet/collision.hpp"
#include "engine/bsa.hpp"
#include "engine/gui/gui.hpp"
#include "engine/resolvers/interior_cell_resolver.hpp"
#include "engine/resolvers/door_resolver.hpp"
#include "engine/resolvers/light_resolver.hpp"
#include "engine/resolvers/static_resolver.hpp"
#include "engine/nifloader/mesh_loader.hpp"
#include "engine/nifloader/collision_object_loader.hpp"
#include "engine/player_controller.hpp"
#include "fs/path.hpp"
#include "ogrebullet/debug_drawer.hpp"
#include "ogrebullet/collision_object_manager.hpp"
#include "ogrebullet/rigid_body.hpp"
#include "ogre/text_resource_manager.hpp"
#include "ogre/window.hpp"
#include "sdl/sdl.hpp"
#include <boost/format.hpp>
#include <Ogre.h>
#include <SDL2/SDL.h>
#include <spdlog/spdlog.h>
#include <memory>
#include <string>

namespace engine {

class Application : public Ogre::FrameListener {
 private:
  std::unique_ptr<BsaArchiveFactory> bsaArchiveFactory{};
  std::unique_ptr<Ogre::RigidBodyFactory> rigidBodyFactory{};

  std::ifstream esmStream;

  std::shared_ptr<spdlog::logger> logger{};
  std::unique_ptr<Ogre::LogManager> ogreLogMgr{};
  std::unique_ptr<Ogre::LogListener> ogreLogListener{};

  std::unique_ptr<Ogre::Root> ogreRoot{};
  sdl::Init sdlInit;

  sdl::WindowPtr sdlWindow{nullptr, nullptr};
  Ogre::RenderWindowPtr ogreWindow;

  std::unique_ptr<bullet::Configuration> bulletConf{};

  nifloader::MeshLoader nifLoader{};
  nifloader::CollisionObjectLoader nifCollisionLoader{};

  std::unique_ptr<Ogre::CollisionObjectManager> collisionObjectMgr{};
  std::unique_ptr<Ogre::TextResourceManager> textResourceMgr{};

  std::unique_ptr<gui::LoadingMenu> menuLoadingMenu{};

  std::unique_ptr<DoorResolver> doorRes{};
  std::unique_ptr<LightResolver> lightRes{};
  std::unique_ptr<StaticResolver> staticRes{};
  std::unique_ptr<InteriorCellResolver> interiorCellRes{};

  std::shared_ptr<InteriorCell> currentCell{};
  bullet::CollisionCaller collisionCaller{};
  std::unique_ptr<PlayerController> playerController{};

  bool drawBulletDebug{false};
  std::unique_ptr<Ogre::DebugDrawer> debugDrawer{};

 public:
  explicit Application(std::string windowName);

  // Set up the logger. Ogre's logging facilities are pretty good but fall down
  // when it comes to formatting. Using boost::format gets pretty tedious so
  // we use spdlog, which has the fmt library built in. Obviously we still want
  // Ogre's internal log messages though, so we use a LogListener to intercept
  // the standard Ogre log messages and hand them over to spdlog.
  void createLoggers();

  void loadIniConfiguration();

  void setRenderSystem(const std::string &systemName);

  void createWindow(const std::string &windowName);

  std::vector<fs::Path> parseBsaList(const fs::Path &masterPath,
                                     const std::string &list);

  // Detect the resource type of path, and declare it with the correct manual
  // resource loader, if any.
  void declareResource(const fs::Path &path, const std::string &resourceGroup);

  // Add the given bsa archive as a resource location
  void declareBsaArchive(const fs::Path &bsaFilename);

  // Declare all the resources in the given bsa archive
  void declareBsaResources(const fs::Path &bsaFilename);

  void pollEvents();

  void dispatchCollisions();

  FormID getCrosshairRef();

  void enableBulletDebugDraw(bool enable);

  Ogre::Root *getRoot() {
    return ogreRoot.get();
  }

  bool frameStarted(const Ogre::FrameEvent &event) override;
  bool frameRenderingQueued(const Ogre::FrameEvent &event) override;
  bool frameEnded(const Ogre::FrameEvent &event) override;
};

} // namespace engine

#endif // OPENOBLIVION_ENGINE_APPLICATION_HPP
