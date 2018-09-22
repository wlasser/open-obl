#ifndef OPENOBLIVION_ENGINE_APPLICATION_HPP
#define OPENOBLIVION_ENGINE_APPLICATION_HPP

#include "engine/bsa.hpp"
#include "engine/bullet/configuration.hpp"
#include "engine/managers/interior_cell_manager.hpp"
#include "engine/managers/light_manager.hpp"
#include "engine/managers/static_manager.hpp"
#include "engine/nifloader/loader.hpp"
#include "engine/ogre/rigid_body_manager.hpp"
#include "engine/player_controller.hpp"
#include <boost/format.hpp>
#include <Ogre.h>
#include <SDL2/SDL.h>
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
using OgreWindowPtr = std::unique_ptr<Ogre::RenderWindow,
                                      std::function<void(Ogre::RenderWindow *)>>;

auto makeSDLWindow(const std::string &windowName, int width, int height,
                   uint32_t flags);

auto makeOgreWindow(Ogre::Root *root,
                    const std::string &windowName,
                    unsigned int width,
                    unsigned int height,
                    std::map<std::string, std::string> *params);

class Application : public Ogre::FrameListener {
 private:
  // This must have a longer lifetime than ogreRoot
  std::unique_ptr<engine::BSAArchiveFactory> bsaArchiveFactory{};

  std::ifstream esmStream;

  std::unique_ptr<Ogre::Root> ogreRoot{};
  Ogre::LogManager *logger{};
  SDLInit sdlInit;

  SDLWindowPtr sdlWindow{nullptr, nullptr};
  OgreWindowPtr ogreWindow;

  std::unique_ptr<engine::bullet::Configuration> bulletConf{};

  nifloader::Loader nifLoader{};
  std::unique_ptr<Ogre::RigidBodyManager> rigidBodyMgr{};
  const std::string resourceGroup = "OOResource";

  std::unique_ptr<engine::LightManager> lightMgr{};
  std::unique_ptr<engine::StaticManager> staticMgr{};
  std::unique_ptr<engine::InteriorCellManager> interiorCellMgr{};

  std::unique_ptr<engine::PlayerController> playerController{};

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
