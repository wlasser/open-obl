#include "bullet/collision.hpp"
#include "engine/application.hpp"
#include "engine/conversions.hpp"
#include "engine/initial_processor.hpp"
#include "engine/keep_strategy.hpp"
#include "engine/managers/interior_cell_manager.hpp"
#include "engine/managers/static_manager.hpp"
#include "ogre/spdlog_listener.hpp"
#include "ogre/window.hpp"
#include "engine/settings.hpp"
#include "esp.hpp"
#include "game_settings.hpp"
#include "sdl/sdl.hpp"
#include <boost/algorithm/string.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <string>

namespace engine {

namespace fs = std::filesystem;

Application::Application(std::string windowName) : FrameListener() {
  createLoggers();
  logger = spdlog::get(settings::log);

  loadIniConfiguration();
  auto &gameSettings = GameSettings::getSingleton();

  // Override the logger levels with user-provided ones, if any
  if (const auto level = gameSettings.get<std::string>("Debug.sOgreLogLevel")) {
    spdlog::get(settings::ogreLog)->set_level(spdlog::level::from_str(*level));
  }
  if (const auto level = gameSettings.get<std::string>("Debug.sLogLevel")) {
    spdlog::get(settings::log)->set_level(spdlog::level::from_str(*level));
  }

  // Start Ogre and set the rendering system
  ogreRoot = std::make_unique<Ogre::Root>("plugins.cfg", "", "");
  setRenderSystem("OpenGL 3+ Rendering Subsystem");
  ogreRoot->initialise(false);

  // Create the window
  sdlInit = sdl::Init();
  createWindow(windowName);

  // Construct the Bullet configuration
  bulletConf = std::make_unique<bullet::Configuration>();

  // Add the resource managers
  collisionObjectMgr = std::make_unique<Ogre::CollisionObjectManager>();

  // Add the factories
  rigidBodyFactory = std::make_unique<Ogre::RigidBodyFactory>();
  ogreRoot->addMovableObjectFactory(rigidBodyFactory.get());

  // Add the main resource group
  const std::string resourceGroup{settings::resourceGroup};
  auto &resGrpMgr = Ogre::ResourceGroupManager::getSingleton();
  resGrpMgr.createResourceGroup(resourceGroup);

  // Register the BSA archive format
  auto &archiveMgr = Ogre::ArchiveManager::getSingleton();
  bsaArchiveFactory = std::make_unique<engine::BSAArchiveFactory>();
  archiveMgr.addArchiveFactory(bsaArchiveFactory.get());

  // Grab the data folder from the ini file
  std::filesystem::path masterPath{conversions::normalizePath(
      gameSettings.get("General.SLocalMasterPath", "Data/"))};

  // Loading from the filesystem is favoured over bsa files so that mods can
  // replace data. Marking the folder as recursive means that files are
  // identified by their full path.
  // The FileSystem archive has platform-dependent behaviour, and in particular
  // is case-sensitive on *nix, but we need case-insensitivity on all platforms.
  // Files must therefore have lowercase names to correctly override.
  // TODO: Replace FileSystem with a case-insensitive version
  resGrpMgr.addResourceLocation(masterPath, "FileSystem", resourceGroup, true);

  // Get list of bsa files from ini
  const auto bsaList{gameSettings.get("Archive.sArchiveList", "")};
  const auto bsaFilenames = parseBSAList(masterPath, bsaList);

  // Meshes need to be declared explicitly as they use a ManualResourceLoader.
  // We could just use Archive.SMasterMeshesArchiveFileName, but it is not
  // guaranteed to match any of Archive.sArchiveList, and also will not work for
  // any mod bsa files. While we're at it, we'll add the bsa files as resource
  // locations and declare every other recognised resource too.
  for (const auto &bsa : bsaFilenames) {
    resGrpMgr.addResourceLocation(bsa, "BSA", resourceGroup);
    Ogre::StringVectorPtr files = archiveMgr.load(bsa, "BSA", true)->list();

    for (const auto &filename : *files) {
      std::filesystem::path path{conversions::normalizePath(filename)};
      auto ext{path.extension()};
      if (ext == ".nif") {
        resGrpMgr.declareResource(path, "Mesh", resourceGroup, &nifLoader);
        resGrpMgr.declareResource(path, "CollisionObject", resourceGroup,
                                  &nifCollisionLoader);
      } else if (ext == ".dds") {
        resGrpMgr.declareResource(path, "Texture", resourceGroup);
      }
    }
  }

  // Shaders are not stored in the data folder (mostly for vcs reasons)
  resGrpMgr.addResourceLocation("./shaders", "FileSystem", resourceGroup);

  // All resources have been declared by now, so we can initialise the resource
  // groups. This won't initialise the default groups.
  resGrpMgr.initialiseAllResourceGroups();

  // Open the main esm
  std::filesystem::path masterEsm{"Oblivion.esm"};
  esmStream = std::ifstream(masterPath / masterEsm, std::ios::binary);
  if (!esmStream.is_open()) {
    throw std::runtime_error(boost::str(
        boost::format("Failed to open %s") % masterEsm.string()));
  }
  logger->info("Opened {}", masterEsm.string());

  // Create the engine managers
  lightMgr = std::make_unique<LightManager>();
  staticMgr = std::make_unique<StaticManager>();
  interiorCellMgr = std::make_unique<InteriorCellManager>(
      esmStream,
      lightMgr.get(),
      staticMgr.get(),
      bulletConf.get(),
      std::make_unique<strategy::KeepCurrent<InteriorCell>>());

  // Read the main esm
  InitialProcessor initialProcessor(lightMgr.get(),
                                    staticMgr.get(),
                                    interiorCellMgr.get());
  esp::readEsp(esmStream, initialProcessor);
  logger->info("Read {}", masterEsm.string());

  // Load a test cell
  currentCell = interiorCellMgr->get(0x00'031b59);
  logger->info("Loaded test cell");

  playerController =
      std::make_unique<engine::PlayerController>(currentCell->scnMgr);
  currentCell->physicsWorld->addRigidBody(playerController->getRigidBody());
  collisionCaller.addCallback(
      playerController->getRigidBody(),
      [this](const auto *other, const auto &contact) {
        playerController->handleCollision(other, contact);
      });

  ogreWindow->addViewport(playerController->getCamera());

  auto startPos = conversions::fromBSCoordinates({-1954.8577f,
                                                  -473.5773f,
                                                  -318.9890f});
  startPos.y += 4.0f;
  playerController->moveTo(startPos);

  debugDrawer = std::make_unique<Ogre::DebugDrawer>(currentCell->scnMgr,
                                                    resourceGroup);
  currentCell->scnMgr->getRootSceneNode()->createChildSceneNode()
      ->attachObject(debugDrawer->getObject());
  debugDrawer->enable(false);
}

void Application::createLoggers() {
  // The console gets info and above, in particular not debug
  auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  consoleSink->set_level(spdlog::level::info);

  // The log file gets everything
  auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
      "OpenOblivion.log", true);
  fileSink->set_level(spdlog::level::trace);

  // Every log will write to both the console and the log file
  const std::initializer_list<spdlog::sink_ptr> sinks{consoleSink, fileSink};

  // Construct the default Ogre logger and register its spdlog listener
  auto ogreLogger = std::make_shared<spdlog::logger>(settings::ogreLog, sinks);
  spdlog::register_logger(ogreLogger);
  ogreLogMgr = std::make_unique<Ogre::LogManager>();
  auto *defaultLog = ogreLogMgr->createLog("Default", true, true, true);
  ogreLogListener = std::make_unique<Ogre::SpdlogListener>(settings::ogreLog);
  defaultLog->addListener(ogreLogListener.get());

  // Construct our own logger
  auto logger = std::make_shared<spdlog::logger>(settings::log, sinks);
  spdlog::register_logger(logger);

  // Set the starting logger levels. These will be modified later according to
  // user settings, if specified.
  logger->set_level(spdlog::level::warn);
  ogreLogger->set_level(spdlog::level::info);
}

void Application::loadIniConfiguration() {
  auto &gameSettings = GameSettings::getSingleton();

  logger->info("Parsing {}", settings::defaultIni);
  gameSettings.load(settings::defaultIni, true);

  if (std::filesystem::is_regular_file(settings::userIni)) {
    logger->info("Parsing {}", settings::userIni);
    gameSettings.load(settings::userIni, true);
  } else {
    logger->warn("User configuration {} not found", settings::userIni);
  }
}

void Application::setRenderSystem(const std::string &systemName) {
  if (auto *renderSystem = ogreRoot->getRenderSystemByName(systemName)) {
    ogreRoot->setRenderSystem(renderSystem);
  } else {
    // List the available render systems
    logger->error("Render system {} not found", systemName);
    logger->info("Available render systems are:");
    for (const auto &system : ogreRoot->getAvailableRenderers()) {
      logger->info(" * {}", system->getName());
    }
    throw std::runtime_error("Invalid render system");
  }
}

void Application::createWindow(const std::string &windowName) {
  auto &gameSettings = GameSettings::getSingleton();

  // Grab the window dimensions
  const int windowWidth = gameSettings.iGet("Display.iSize W");
  const int windowHeight = gameSettings.iGet("Display.iSize H");
  if (windowHeight <= 0 || windowWidth <= 0) {
    logger->critical("Cannot create a window with width {} and height {}",
                     windowWidth, windowHeight);
    logger->critical(
        "Set 'Display.iSize W' and 'Display.iSize H' to sensible values");
    throw std::runtime_error("Cannot create window with negative size");
  }

  // Grab the window settings
  sdl::WindowFlags windowFlags{};
  if (gameSettings.bGet("Display.bFull Screen")) {
    windowFlags |= sdl::WindowFlags::Fullscreen;
  }

  // Make the window and find its system parent handle
  sdlWindow = sdl::makeWindow(windowName, windowWidth, windowHeight,
                              windowFlags);
  const auto sdlWindowInfo = sdl::getSysWMInfo(sdlWindow.get());
  const auto parent = sdl::getWindowParent(sdlWindowInfo);

  // Make cursor behaviour more sensible
  sdl::setRelativeMouseMode(true);

  // Construct a render window with the SDL window as a parent; SDL handles the
  // window itself, Ogre manages the OpenGL context.
  std::map<std::string, std::string> params = {
      {"parentWindowHandle", parent}
  };
  ogreWindow = Ogre::makeRenderWindow(ogreRoot.get(),
                                      windowName,
                                      static_cast<const unsigned>(windowWidth),
                                      static_cast<const unsigned>(windowHeight),
                                      &params);
}

std::vector<fs::path>
Application::parseBSAList(const fs::path &masterPath, const std::string &list) {
  std::vector<std::string> names{};

  // Split them on commas, this leaves trailing whitespace
  boost::split(names, list, [](char c) { return c == ','; });

  // Trim the whitespace and append the data folder
  std::vector<fs::path> filenames(names.size());
  std::transform(names.begin(), names.end(), filenames.begin(),
                 [&masterPath](std::string name) {
                   boost::trim(name);
                   return masterPath / fs::path{name};
                 });

  // Reject any invalid ones
  filenames.erase(std::remove_if(filenames.begin(), filenames.end(),
                                 [](const auto &filename) {
                                   return !fs::is_regular_file(filename);
                                 }),
                  filenames.end());

  return filenames;
}

void Application::pollEvents() {
  sdl::Event sdlEvent;
  while (sdl::pollEvent(sdlEvent)) {
    switch (sdl::typeOf(sdlEvent)) {

      case sdl::EventType::Quit: {
        ogreRoot->queueEndRendering();
        break;
      }

      case sdl::EventType::KeyDown: {
        if (sdlEvent.key.repeat) break;
        switch (sdl::keyCodeOf(sdlEvent.key)) {

          case sdl::KeyCode::Escape: {
            ogreRoot->queueEndRendering();
            break;
          }

          case sdl::KeyCode::H: {
            drawBulletDebug = !drawBulletDebug;
            enableBulletDebugDraw(drawBulletDebug);
            break;
          }

          case sdl::KeyCode::A: {
            playerController->handleEvent({MoveEvent::Left, true, 0.0f});
            break;
          }

          case sdl::KeyCode::D: {
            playerController->handleEvent({MoveEvent::Right, true, 0.0f});
            break;
          }

          case sdl::KeyCode::W: {
            playerController->handleEvent({MoveEvent::Forward, true, 0.0f});
            break;
          }

          case sdl::KeyCode::S: {
            playerController->handleEvent({MoveEvent::Backward, true, 0.0f});
            break;
          }

          case sdl::KeyCode::Space: {
            playerController->handleEvent({MoveEvent::Jump, false, 0.0f});
            break;
          }

          default:break;
        }
        break;
      }

      case sdl::EventType::KeyUp: {
        if (sdlEvent.key.repeat) break;
        switch (sdl::keyCodeOf(sdlEvent.key)) {

          case sdl::KeyCode::A: {
            playerController->handleEvent({MoveEvent::Left, false, 0.0f});
            break;
          }

          case sdl::KeyCode::D: {
            playerController->handleEvent({MoveEvent::Right, false, 0.0f});
            break;
          }

          case sdl::KeyCode::W: {
            playerController->handleEvent({MoveEvent::Forward, false, 0.0f});
            break;
          }

          case sdl::KeyCode::S: {
            playerController->handleEvent({MoveEvent::Backward, false, 0.0f});
            break;
          }
          default:break;
        }
        break;
      }

      case sdl::EventType::MouseMotion: {
        playerController->handleEvent(
            MoveEvent(MoveEvent::Pitch, true, sdlEvent.motion.yrel));
        playerController->handleEvent(
            MoveEvent(MoveEvent::Yaw, true, sdlEvent.motion.xrel));
        break;
      }

      case sdl::EventType::WindowEvent: {
        if (sdl::typeOf(sdlEvent.window) == sdl::WindowEventType::Resized) {
          ogreWindow->resize(static_cast<unsigned int>(sdlEvent.window.data1),
                             static_cast<unsigned int>(sdlEvent.window.data2));
          ogreWindow->windowMovedOrResized();
        }
        break;
      }

      default:break;
    }
  }
}

void Application::dispatchCollisions() {
  gsl::not_null dispatcher{dynamic_cast<btCollisionDispatcher *>(
                               currentCell->physicsWorld->getDispatcher())};
  collisionCaller.runCallbacks(dispatcher);
}

void Application::enableBulletDebugDraw(bool enable) {
  if (enable) {
    currentCell->physicsWorld->setDebugDrawer(debugDrawer.get());
  } else {
    currentCell->physicsWorld->setDebugDrawer(nullptr);
    debugDrawer->clearLines();
    debugDrawer->build();
  }

  debugDrawer->enable(enable);
}

bool Application::frameStarted(const Ogre::FrameEvent &event) {
  pollEvents();
  playerController->update(event.timeSinceLastFrame);

  currentCell->physicsWorld->stepSimulation(event.timeSinceLastFrame);

  dispatchCollisions();

  if (drawBulletDebug) {
    debugDrawer->clearLines();
    currentCell->physicsWorld->debugDrawWorld();
    debugDrawer->build();
  }

  return true;
}

bool Application::frameRenderingQueued(const Ogre::FrameEvent &event) {
  return true;
}

bool Application::frameEnded(const Ogre::FrameEvent &event) {
  return true;
}

} // namespace engine
