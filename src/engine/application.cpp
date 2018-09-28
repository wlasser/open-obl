#include "engine/application.hpp"
#include "engine/conversions.hpp"
#include "engine/initial_processor.hpp"
#include "engine/keep_strategy.hpp"
#include "engine/managers/interior_cell_manager.hpp"
#include "engine/managers/static_manager.hpp"
#include "engine/ogre/spdlog_listener.hpp"
#include "engine/ogre/window.hpp"
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

Application::Application(std::string windowName) : FrameListener() {
  // Set up the logger. Ogre's logging facilities are pretty good but fall down
  // when it comes to formatting. Using boost::format gets pretty tedious so
  // we use spdlog, which has the fmt library built in. Obviously we still want
  // Ogre's internal log messages though, so we use a LogListener to intercept
  // the standard Ogre log messages and hand them over to spdlog.

  // The console gets info and above, in particular not debug
  auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  consoleSink->set_level(spdlog::level::info);

  // The log file gets everything
  auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
      "OpenOblivion.log", true);
  fileSink->set_level(spdlog::level::trace);

  // Every log will write to both the console and the log file
  std::initializer_list<spdlog::sink_ptr> sinks{consoleSink, fileSink};

  // Construct the default Ogre logger and register its spdlog listener
  auto ogreLogger = std::make_shared<spdlog::logger>(settings::ogreLog, sinks);
  spdlog::register_logger(ogreLogger);
  ogreLogMgr = std::make_unique<Ogre::LogManager>();
  auto *defaultLog = ogreLogMgr->createLog("Default", true, true, true);
  ogreLogListener = std::make_unique<Ogre::SpdlogListener>(settings::ogreLog);
  defaultLog->addListener(ogreLogListener.get());

  // Construct our own logger
  logger = std::make_shared<spdlog::logger>(settings::log, sinks);
  spdlog::register_logger(logger);

  // Set the starting logger levels
  spdlog::get(settings::ogreLog)->set_level(spdlog::level::warn);
  spdlog::get(settings::log)->set_level(spdlog::level::info);

  // Load the configuration files
  auto &gameSettings = GameSettings::getSingleton();
  std::filesystem::path defaultIni = "Oblivion_default.ini";
  std::filesystem::path userIni = "Oblivion.ini";
  logger->info("Parsing {}", defaultIni.string());
  gameSettings.load(defaultIni, true);
  if (std::filesystem::is_regular_file(userIni)) {
    logger->info("Parsing {}", userIni.string());
    gameSettings.load("Oblivion.ini", true);
  } else {
    logger->warn("User configuration {} not found", userIni.string());
  }

  // Override the logger levels with user-provided ones, if any
  if (auto level = gameSettings.get<std::string>("Debug.sOgreLogLevel")) {
    spdlog::get(settings::ogreLog)->set_level(spdlog::level::from_str(*level));
  }
  if (auto level = gameSettings.get<std::string>("Debug.sLogLevel")) {
    spdlog::get(settings::log)->set_level(spdlog::level::from_str(*level));
  }

  // Start Ogre
  ogreRoot = std::make_unique<Ogre::Root>("plugins.cfg", "", "");

  // Choose a render system
  std::string renderSystemName = "OpenGL 3+ Rendering Subsystem";
  if (auto *renderSystem = ogreRoot->getRenderSystemByName(renderSystemName)) {
    ogreRoot->setRenderSystem(renderSystem);
  } else {
    // List the available render systems
    logger->error("Render system {} not found", renderSystemName);
    logger->info("Available render systems are:");
    for (const auto &system : ogreRoot->getAvailableRenderers()) {
      logger->info(" * {}", system->getName());
    }
    throw std::runtime_error("Invalid render system");
  }

  // Initialise the rendering component of Ogre
  ogreRoot->initialise(false);

  // Initialise SDL
  sdlInit = sdl::Init();

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
  auto sdlWindowInfo = sdl::getSysWMInfo(sdlWindow.get());
  auto parent = sdl::getWindowParent(sdlWindowInfo);

  // Make cursor behaviour more sensible
  sdl::setRelativeMouseMode(true);

  // Construct a render window with the SDL window as a parent; SDL handles the
  // window itself, Ogre manages the OpenGL context.
  std::map<std::string, std::string> params = {{"parentWindowHandle", parent}};
  ogreWindow = Ogre::makeRenderWindow(ogreRoot.get(),
                                      windowName,
                                      static_cast<const unsigned>(windowWidth),
                                      static_cast<const unsigned>(windowHeight),
                                      &params);

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
  auto bsaList{gameSettings.get("Archive.sArchiveList", "")};
  std::vector<std::string> bsaNames{};

  // Split them on commas, this leaves trailing whitespace
  boost::split(bsaNames, bsaList, [](char c) { return c == ','; });

  // Trim the whitespace and append the data folder
  std::vector<std::filesystem::path> bsaFilenames(bsaNames.size());
  std::transform(bsaNames.begin(), bsaNames.end(), bsaFilenames.begin(),
                 [&masterPath](std::string name) {
                   boost::trim(name);
                   return masterPath / std::filesystem::path{name};
                 });

  // Reject any invalid ones and add the others as resource locations
  for (auto it = bsaFilenames.begin(); it != bsaFilenames.end();) {
    if (!std::filesystem::is_regular_file(*it)) {
      std::swap(*it, bsaFilenames.back());
      bsaFilenames.pop_back();
    } else {
      resGrpMgr.addResourceLocation(*it, "BSA", resourceGroup);
      ++it;
    }
  }

  // Meshes need to be declared explicitly as they use a ManualResourceLoader.
  // We could just use Archive.SMasterMeshesArchiveFileName, but it is not
  // guaranteed to match any of Archive.sArchiveList, and also will not work for
  // any mod bsa files. While we're at it, we'll declare every other recognised
  // resource too.
  for (const auto &bsa : bsaFilenames) {
    Ogre::StringVectorPtr files = archiveMgr.load(bsa, "BSA", true)->list();
    for (const auto &filename : *files) {
      std::filesystem::path path{conversions::normalizePath(filename)};
      auto ext{path.extension()};
      if (ext == ".nif") {
        resGrpMgr.declareResource(path, "Mesh", resourceGroup, &nifLoader);
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

  // Create a scene manager
  auto *scnMgr = ogreRoot->createSceneManager();

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

bool Application::frameStarted(const Ogre::FrameEvent &event) {
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
            if (drawHavok) {
              currentCell->physicsWorld->setDebugDrawer(nullptr);
              debugDrawer->clearLines();
              debugDrawer->build();
            } else {
              currentCell->physicsWorld->setDebugDrawer(debugDrawer.get());
            }
            drawHavok = !drawHavok;
            debugDrawer->enable(drawHavok);
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

  playerController->update(event.timeSinceLastFrame);

  currentCell->physicsWorld->stepSimulation(1.0f / 60.0f);

  if (drawHavok) {
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
