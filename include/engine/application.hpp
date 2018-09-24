#ifndef OPENOBLIVION_ENGINE_APPLICATION_HPP
#define OPENOBLIVION_ENGINE_APPLICATION_HPP

#include "engine/bsa.hpp"
#include "engine/bullet/configuration.hpp"
#include "engine/managers/interior_cell_manager.hpp"
#include "engine/managers/light_manager.hpp"
#include "engine/managers/static_manager.hpp"
#include "engine/nifloader/loader.hpp"
#include "engine/ogre/debug_drawer.hpp"
#include "engine/ogre/rigid_body_manager.hpp"
#include "engine/ogre/window.hpp"
#include "engine/player_controller.hpp"
#include <boost/format.hpp>
#include <Ogre.h>
#include <SDL2/SDL.h>
#include <spdlog/spdlog.h>
#include <memory>
#include <string>

namespace engine {

// RAII wrapper for SDL, calls SDL_Init on construction and SDL_Quit on
// destruction for automatic cleanup.
struct SDLInit {
  SDLInit() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
      throw std::runtime_error(boost::str(
          boost::format("SDL_Init failed: %s") % SDL_GetError()));
    }
  }

  ~SDLInit() {
    SDL_Quit();
  }
};

using SDLWindowPtr = std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>;

auto makeSDLWindow(const std::string &windowName, int width, int height,
                   uint32_t flags);

class Application : public Ogre::FrameListener {
 private:
  std::unique_ptr<BSAArchiveFactory> bsaArchiveFactory{};

  std::ifstream esmStream;

  std::shared_ptr<spdlog::logger> logger{};
  std::unique_ptr<Ogre::LogManager> ogreLogger{};
  std::unique_ptr<Ogre::LogListener> ogreLogListener{};

  std::unique_ptr<Ogre::Root> ogreRoot{};
  SDLInit sdlInit;

  SDLWindowPtr sdlWindow{nullptr, nullptr};
  Ogre::RenderWindowPtr ogreWindow;

  std::unique_ptr<bullet::Configuration> bulletConf{};

  nifloader::Loader nifLoader{};
  std::unique_ptr<Ogre::RigidBodyManager> rigidBodyMgr{};
  const std::string resourceGroup = "OOResource";

  std::unique_ptr<LightManager> lightMgr{};
  std::unique_ptr<StaticManager> staticMgr{};
  std::unique_ptr<InteriorCellManager> interiorCellMgr{};

  std::shared_ptr<InteriorCell> currentCell{};
  std::unique_ptr<PlayerController> playerController{};
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
