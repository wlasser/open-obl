#ifndef OPENOBLIVION_ENGINE_APPLICATION_HPP
#define OPENOBLIVION_ENGINE_APPLICATION_HPP

#include "engine/bsa.hpp"
#include "engine/bullet/configuration.hpp"
#include "engine/managers/interior_cell_manager.hpp"
#include "engine/managers/light_manager.hpp"
#include "engine/managers/static_manager.hpp"
#include "engine/nifloader/loader.hpp"
#include "engine/ogre/debug_drawer.hpp"
#include "engine/ogre/collision_object_manager.hpp"
#include "engine/ogre/rigid_body.hpp"
#include "engine/ogre/window.hpp"
#include "engine/player_controller.hpp"
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
  std::unique_ptr<BSAArchiveFactory> bsaArchiveFactory{};
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

  nifloader::Loader nifLoader{};
  std::unique_ptr<Ogre::CollisionObjectManager> collisionObjectMgr{};

  std::unique_ptr<LightManager> lightMgr{};
  std::unique_ptr<StaticManager> staticMgr{};
  std::unique_ptr<InteriorCellManager> interiorCellMgr{};

  std::shared_ptr<InteriorCell> currentCell{};
  std::unique_ptr<PlayerController> playerController{};

  bool drawHavok{false};
  std::unique_ptr<Ogre::DebugDrawer> debugDrawer{};

 public:
  explicit Application(std::string windowName);

  Ogre::Root *getRoot() {
    return ogreRoot.get();
  }

  bool frameStarted(const Ogre::FrameEvent &event) override;
  bool frameRenderingQueued(const Ogre::FrameEvent &event) override;
  bool frameEnded(const Ogre::FrameEvent &event) override;
};

} // namespace engine

#endif // OPENOBLIVION_ENGINE_APPLICATION_HPP
