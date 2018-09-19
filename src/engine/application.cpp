#include "engine/application.hpp"
#include "engine/cell_manager.hpp"
#include "engine/conversions.hpp"
#include "engine/initial_processor.hpp"
#include "engine/keep_strategy.hpp"
#include "engine/static_manager.hpp"
#include "esp.hpp"
#include "SDL.h"
#include "SDL_syswm.h"
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <string>

namespace engine {

auto makeSDLWindow(const std::string &windowName, int width, int height,
                   uint32_t flags) {
  auto win = SDL_CreateWindow(windowName.c_str(),
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              width, height,
                              flags);
  if (win == nullptr) {
    throw std::runtime_error(boost::str(
        boost::format("SDL_CreateWindow failed: %s") % SDL_GetError()));
  }

  return SDLWindowPtr(win, SDL_DestroyWindow);
}

auto makeOgreWindow(Ogre::Root *root,
                    const std::string &windowName,
                    unsigned int width,
                    unsigned int height,
                    std::map<std::string, std::string> *params) {
  auto win = root->createRenderWindow(windowName, width, height, false, params);
  if (win == nullptr) {
    throw std::runtime_error("Failed to create Ogre::RenderWindow");
  }

  return OgreWindowPtr(win, [root](auto *target) {
    root->destroyRenderTarget(target);
  });
}

Application::Application(std::string windowName) : FrameListener() {
  // Start Ogre
  ogreRoot = std::make_unique<Ogre::Root>("plugins.cfg", "",
                                          "OpenOblivion.log");

  // Set up the logger
  logger = Ogre::LogManager::getSingletonPtr();

  // Choose a render system
  std::string renderSystemName = "OpenGL 3+ Rendering Subsystem";
  if (auto *renderSystem = ogreRoot->getRenderSystemByName(renderSystemName)) {
    ogreRoot->setRenderSystem(renderSystem);
  } else {
    // List the available render systems
    auto notFound = boost::str(boost::format("Render system %s not found")
                                   % renderSystemName);
    logger->logMessage(notFound);
    logger->logMessage("Available render systems are:");
    auto renderSystemFmt = boost::format(" - %s");
    for (const auto &system : ogreRoot->getAvailableRenderers()) {
      logger->logMessage(boost::str(renderSystemFmt % system->getName()));
    }
    throw std::runtime_error(notFound);
  }

  // Initialise the rendering component of Ogre
  ogreRoot->initialise(false);

  // Initialise SDL, create an SDL window and Ogre window
  sdlInit = SDLInit();

  const auto windowWidth = 1600;
  const auto windowHeight = 900;
  uint32_t flags = SDL_WINDOW_RESIZABLE;

  sdlWindow = makeSDLWindow(windowName, windowWidth, windowHeight, flags);

  SDL_SysWMinfo sdlWindowInfo{};
  // This tells SDL we have version 2, otherwise the next call fails regardless
  // of the actual SDL version.
  SDL_VERSION(&sdlWindowInfo.version);
  if (!SDL_GetWindowWMInfo(sdlWindow.get(), &sdlWindowInfo)) {
    throw std::runtime_error(boost::str(
        boost::format("SDL_GetWindowWMInfo failed: %s") % SDL_GetError()));
  }
  auto parent = std::to_string(sdlWindowInfo.info.x11.window);

  // Make cursor behaviour more sensible
  if (SDL_SetRelativeMouseMode(SDL_TRUE)) {
    throw std::runtime_error(boost::str(
        boost::format("SDL_SetRelativeMouseMode failed: %s") % SDL_GetError()));
  }

  std::map<std::string, std::string> params = {{"parentWindowHandle", parent}};
  ogreWindow = makeOgreWindow(ogreRoot.get(), windowName, windowWidth,
                              windowHeight, &params);

  // Add the main resource group
  auto &resGrpMgr = Ogre::ResourceGroupManager::getSingleton();
  resGrpMgr.createResourceGroup(resourceGroup);

  // Register the BSA archive format
  auto &archiveMgr = Ogre::ArchiveManager::getSingleton();
  bsaArchiveFactory = std::make_unique<engine::BSAArchiveFactory>();
  archiveMgr.addArchiveFactory(bsaArchiveFactory.get());

  // Favour reading from the filesystem (so that mods can overwrite data), then
  // register all bsa files in the data folder. This allows mods to use their
  // own bsa files, if they want.
  resGrpMgr.addResourceLocation("./Data", "FileSystem", resourceGroup, true);
  //for (const auto &entry : std::filesystem::directory_iterator("./Data")) {
  //  if (entry.is_regular_file() && entry.path().extension() == ".bsa") {
  //    resGrpMgr.addResourceLocation(entry.path(), "BSA", resourceGroup);
  //  }
  //}
  std::vector<std::filesystem::path> testBSAs = {
      "./Data/Oblivion - Meshes.bsa",
      "./Data/Oblivion - Textures - Compressed.bsa"
  };
  for (const auto &entry : testBSAs) {
    resGrpMgr.addResourceLocation(entry, "BSA", resourceGroup);
  }

  auto meshList = archiveMgr.load(testBSAs[0], "BSA", true)->list();
  for (const auto &filename : *meshList) {
    std::filesystem::path path{conversions::normalizePath(filename)};
    if (path.extension() == ".nif") {
      resGrpMgr.declareResource(path, "Mesh", resourceGroup, &nifLoader);
    }
  }
  logger->logMessage("Declared mesh files");

  auto textureList = archiveMgr.load(testBSAs[1], "BSA", true)->list();
  for (const auto &filename : *textureList) {
    std::filesystem::path path{conversions::normalizePath(filename)};
    if (path.extension() == ".dds") {
      resGrpMgr.declareResource(path, "Texture", resourceGroup);
    }
  }
  logger->logMessage("Declared texture files");

  // Declare the shader programs
  resGrpMgr.addResourceLocation("./shaders", "FileSystem", resourceGroup);
  logger->logMessage("Declared shader files");

  resGrpMgr.initialiseAllResourceGroups();

  // Create a scene manager
  auto *scnMgr = ogreRoot->createSceneManager();

  // Open the main esm
  esmStream = std::ifstream("Data/Oblivion.esm", std::ios::binary);
  if (!esmStream.is_open()) {
    throw std::runtime_error("Failed to open esm");
  }
  logger->logMessage("Opened Oblivion.esm");

  // Create the engine managers
  lightMgr = std::make_unique<LightManager>();
  staticMgr = std::make_unique<StaticManager>();
  interiorCellMgr = std::make_unique<InteriorCellManager>(
      esmStream,
      lightMgr.get(),
      staticMgr.get(),
      std::make_unique<strategy::KeepCurrent<InteriorCell>>());

  // Read the main esm
  InitialProcessor initialProcessor(lightMgr.get(),
                                    staticMgr.get(),
                                    interiorCellMgr.get());
  esp::readEsp(esmStream, initialProcessor);
  logger->logMessage("Read Oblivion.esm");

  // Load a test cell
  auto cell = interiorCellMgr->get(0x00'031b59);
  logger->logMessage("Loaded test cell");

  playerController = std::make_unique<engine::PlayerController>(cell->scnMgr);
  ogreWindow->addViewport(playerController->getCamera());
  playerController->moveTo(conversions::fromBSCoordinates(
      {200.0f, -347.0f, -460.0f}));
}

bool Application::frameStarted(const Ogre::FrameEvent &event) {
  SDL_Event sdlEvent;
  while (SDL_PollEvent(&sdlEvent)) {
    switch (sdlEvent.type) {

      case SDL_QUIT:ogreRoot->queueEndRendering();
        break;

      case SDL_KEYDOWN:if (sdlEvent.key.repeat) break;
        switch (sdlEvent.key.keysym.sym) {
          case SDLK_ESCAPE:ogreRoot->queueEndRendering();
            break;
          case SDLK_a: {
            playerController->sendEvent(
                {PlayerController::MoveEvent::Left, true, 0.0f});
            break;
          }
          case SDLK_d: {
            playerController->sendEvent(
                {PlayerController::MoveEvent::Right, true, 0.0f});
            break;
          }
          case SDLK_w: {
            playerController->sendEvent(
                {PlayerController::MoveEvent::Forward, true, 0.0f});
            break;
          }
          case SDLK_s: {
            playerController->sendEvent(
                {PlayerController::MoveEvent::Backward, true, 0.0f});
            break;
          }
          default:break;
        }
        break;

      case SDL_KEYUP:if (sdlEvent.key.repeat) break;
        switch (sdlEvent.key.keysym.sym) {
          case SDLK_a: {
            playerController->sendEvent(
                {PlayerController::MoveEvent::Left, false, 0.0f});
            break;
          }
          case SDLK_d: {
            playerController->sendEvent(
                {PlayerController::MoveEvent::Right, false, 0.0f});
            break;
          }
          case SDLK_w: {
            playerController->sendEvent(
                {PlayerController::MoveEvent::Forward, false, 0.0f});
            break;
            case SDLK_s: {
              playerController->sendEvent(
                  {PlayerController::MoveEvent::Backward, false, 0.0f});
              break;
            }
          }
          default:break;
        }
        break;

      case SDL_MOUSEMOTION: {
        playerController->sendEvent(PlayerController::MoveEvent(
            PlayerController::MoveEvent::Pitch,
            true,
            sdlEvent.motion.yrel));
        playerController->sendEvent(PlayerController::MoveEvent(
            PlayerController::MoveEvent::Yaw,
            true,
            sdlEvent.motion.xrel));
        break;
      }
      case SDL_WINDOWEVENT:
        if (sdlEvent.window.event == SDL_WINDOWEVENT_RESIZED) {
          ogreWindow->resize(static_cast<unsigned int>(sdlEvent.window.data1),
                             static_cast<unsigned int>(sdlEvent.window.data2));
          ogreWindow->windowMovedOrResized();
        }
      default:break;
    }
  }

  playerController->update(event.timeSinceLastFrame);

  return true;
}

bool Application::frameRenderingQueued(const Ogre::FrameEvent &event) {
  return true;
}

bool Application::frameEnded(const Ogre::FrameEvent &event) {
  return true;
}

} // namespace engine
