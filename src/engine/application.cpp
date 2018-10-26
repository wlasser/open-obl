#include "bullet/collision.hpp"
#include "engine/application.hpp"
#include "engine/conversions.hpp"
#include "engine/initial_processor.hpp"
#include "engine/keep_strategy.hpp"
#include "engine/resolvers/interior_cell_resolver.hpp"
#include "engine/resolvers/static_resolver.hpp"
#include "ogre/ogre_stream_wrappers.hpp"
#include "ogre/spdlog_listener.hpp"
#include "ogre/window.hpp"
#include "ogrebullet/conversions.hpp"
#include "ogreimgui/imgui_manager.hpp"
#include "engine/settings.hpp"
#include "esp.hpp"
#include "fs/path.hpp"
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
  ogreRoot->addFrameListener(this);

  // Create the window
  sdlInit = sdl::Init();
  createWindow(windowName);

  // Set the keyboard configuration
  keyMap = std::make_unique<KeyMap>(gameSettings);

  // Construct the Bullet configuration
  bulletConf = std::make_unique<bullet::Configuration>();

  // Add the resource managers
  collisionObjectMgr = std::make_unique<Ogre::CollisionObjectManager>();
  textResourceMgr = std::make_unique<Ogre::TextResourceManager>();

  // Add the factories
  rigidBodyFactory = std::make_unique<Ogre::RigidBodyFactory>();
  ogreRoot->addMovableObjectFactory(rigidBodyFactory.get());

  // Add the main resource group
  const std::string resourceGroup{settings::resourceGroup};
  auto &resGrpMgr = Ogre::ResourceGroupManager::getSingleton();
  resGrpMgr.createResourceGroup(resourceGroup);

  // Register the BSA archive format
  auto &archiveMgr = Ogre::ArchiveManager::getSingleton();
  bsaArchiveFactory = std::make_unique<engine::BsaArchiveFactory>();
  archiveMgr.addArchiveFactory(bsaArchiveFactory.get());

  // Grab the data folder from the ini file
  const fs::Path dataPath{gameSettings.get("General.SLocalMasterPath", "Data")};

  // Loading from the filesystem is favoured over bsa files so that mods can
  // replace data. Marking the folder as recursive means that files are
  // identified by their full path.
  // The FileSystem archive has platform-dependent behaviour, and in particular
  // is case-sensitive on *nix, but we need case-insensitivity on all platforms.
  // Files must therefore have lowercase names to correctly override.
  // TODO: Replace FileSystem with a case-insensitive version
  resGrpMgr.addResourceLocation(dataPath.c_str(), "FileSystem", resourceGroup,
                                true);

  // Get list of bsa files from ini
  const std::string bsaList{gameSettings.get("Archive.sArchiveList", "")};
  const auto bsaFilenames{parseBsaList(dataPath, bsaList)};

  // Meshes need to be declared explicitly as they use a ManualResourceLoader.
  // We could just use Archive.SMasterMeshesArchiveFileName, but it is not
  // guaranteed to match any of Archive.sArchiveList, and also will not work for
  // any mod bsa files. While we're at it, we'll add the bsa files as resource
  // locations and declare every other recognised resource too.
  for (const auto &bsa : bsaFilenames) {
    declareBsaArchive(bsa);
    declareBsaResources(bsa);
  }

  // Shaders are not stored in the data folder (mostly for vcs reasons)
  resGrpMgr.addResourceLocation("./shaders", "FileSystem", resourceGroup);

  // All resources have been declared by now, so we can initialise the resource
  // groups. This won't initialise the default groups.
  resGrpMgr.initialiseAllResourceGroups();

  // Instantiate the menus
  auto &txtResMgr = Ogre::TextResourceManager::getSingleton();
  std::string menuPath = "menus/loading_menu.xml";
  auto menuPtr = txtResMgr.getByName(menuPath, resourceGroup);
  if (!menuPtr) {
    logger->error("Resource does not exist");
    throw std::runtime_error(boost::str(
        boost::format("Failed to open %s") % menuPath));
  }
  menuPtr->load(false);
  std::stringstream menuStream{menuPtr->getString()};
  //gui::parseMenu(menuStream);
  menuLoadingMenu = std::make_unique<gui::LoadingMenu>();

  // Open the main esm
  const fs::Path masterEsm{"Oblivion.esm"};
  esmStream = std::ifstream((dataPath / masterEsm).sysPath(),
                            std::ios::binary);
  if (!esmStream.is_open()) {
    throw std::runtime_error(boost::str(
        boost::format("Failed to open %s") % masterEsm.view()));
  }
  logger->info("Opened {}", masterEsm.view());

  // Create the engine managers
  doorRes = std::make_unique<DoorResolver>();
  lightRes = std::make_unique<LightResolver>();
  staticRes = std::make_unique<StaticResolver>();
  InteriorCellResolver::resolvers_t resolvers{
      *doorRes, *lightRes, *staticRes
  };
  interiorCellRes = std::make_unique<InteriorCellResolver>(
      esmStream,
      resolvers,
      *bulletConf,
      std::make_unique<strategy::KeepCurrent<InteriorCell>>());

  // Read the main esm
  InitialProcessor initialProcessor(doorRes.get(),
                                    lightRes.get(),
                                    staticRes.get(),
                                    interiorCellRes.get());
  esp::readEsp(esmStream, initialProcessor);
  logger->info("Read {}", masterEsm.view());

  // Load a test cell
  currentCell = interiorCellRes->make(BaseId{0x00'048706});
  //currentCell = interiorCellRes->make(BaseId{0x00'031b59});
  logger->info("Loaded test cell");

  playerController =
      std::make_unique<character::PlayerController>(currentCell->scnMgr);
  currentCell->physicsWorld->addRigidBody(playerController->getRigidBody());
  collisionCaller.addCallback(
      playerController->getRigidBody(),
      [this](const auto *other, const auto &contact) {
        playerController->handleCollision(other, contact);
      });

  ogreWindow->addViewport(playerController->getCamera());
  ogreRoot->getRenderSystem()
      ->_setViewport(playerController->getCamera()->getViewport());

  auto startPos = conversions::fromBSCoordinates({0, 0, 0});
  //auto startPos = conversions::fromBSCoordinates({-1954.8577f, -473.5773f, -318.9890f});
  startPos.y += 4.0f;
  playerController->moveTo(startPos);

  debugDrawer = std::make_unique<Ogre::DebugDrawer>(currentCell->scnMgr,
                                                    resourceGroup);
  currentCell->scnMgr->getRootSceneNode()->createChildSceneNode()
      ->attachObject(debugDrawer->getObject());
  debugDrawer->enable(false);

  imguiMgr = std::make_unique<Ogre::ImGuiManager>();
  currentCell->scnMgr->addRenderQueueListener(imguiMgr.get());
}

void Application::createLoggers() {
  using namespace spdlog::sinks;

  // The console gets info and above, in particular not debug
  auto consoleSink{std::make_shared<stdout_color_sink_mt>()};
  consoleSink->set_level(spdlog::level::info);

  // The log file gets everything
  auto fileSink{std::make_shared<basic_file_sink_mt>("OpenOblivion.log", true)};
  fileSink->set_level(spdlog::level::trace);

  // Every log will write to both the console and the log file
  const std::initializer_list<spdlog::sink_ptr> sinks{consoleSink, fileSink};

  // Construct the default Ogre logger and register its spdlog listener
  auto ogreLogger{std::make_shared<spdlog::logger>(settings::ogreLog, sinks)};
  spdlog::register_logger(ogreLogger);
  ogreLogMgr = std::make_unique<Ogre::LogManager>();
  auto *defaultLog{ogreLogMgr->createLog("Default", true, true, true)};
  ogreLogListener = std::make_unique<Ogre::SpdlogListener>(settings::ogreLog);
  defaultLog->addListener(ogreLogListener.get());

  // Construct our own logger
  auto logger{std::make_shared<spdlog::logger>(settings::log, sinks)};
  spdlog::register_logger(logger);

  // Set the starting logger levels. These will be modified later according to
  // user settings, if specified.
  logger->set_level(spdlog::level::warn);
  ogreLogger->set_level(spdlog::level::info);
}

void Application::loadIniConfiguration() {
  auto &gameSettings{GameSettings::getSingleton()};

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
  const auto &gameSettings{GameSettings::getSingleton()};

  // Grab the window dimensions
  const int windowWidth{gameSettings.iGet("Display.iSize W")};
  const int windowHeight{gameSettings.iGet("Display.iSize H")};
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
  const auto sdlWindowInfo{sdl::getSysWMInfo(sdlWindow.get())};
  const auto parent{sdl::getWindowParent(sdlWindowInfo)};

  // Make cursor behaviour more sensible
  //sdl::setRelativeMouseMode(true);

  // Construct a render window with the SDL window as a parent; SDL handles the
  // window itself, Ogre manages the OpenGL context.
  const std::map<std::string, std::string> params{
      {"parentWindowHandle", parent}
  };
  ogreWindow = Ogre::makeRenderWindow(ogreRoot.get(),
                                      windowName,
                                      static_cast<const unsigned>(windowWidth),
                                      static_cast<const unsigned>(windowHeight),
                                      &params);
}

std::vector<fs::Path>
Application::parseBsaList(const fs::Path &masterPath, const std::string &list) {
  std::vector<std::string> names{};

  // Split them on commas, this leaves trailing whitespace
  boost::split(names, list, [](char c) { return c == ','; });

  // Trim the whitespace and append the data folder
  std::vector<fs::Path> filenames(names.size());
  std::transform(names.begin(), names.end(), filenames.begin(),
                 [&masterPath](std::string name) {
                   boost::trim(name);
                   return masterPath / fs::Path{name};
                 });

  // Reject any invalid ones
  const auto predicate = [](const auto &filename) {
    return !filename.exists();
  };
  filenames.erase(std::remove_if(filenames.begin(), filenames.end(), predicate),
                  filenames.end());

  return filenames;
}

void Application::declareResource(const fs::Path &path,
                                  const std::string &resourceGroup) {
  using namespace std::literals;
  auto &resGrpMgr{Ogre::ResourceGroupManager::getSingleton()};
  const auto ext{path.extension()};

  if (ext == "nif"sv) {
    resGrpMgr.declareResource(path.c_str(), "Mesh",
                              resourceGroup, &nifLoader);
    resGrpMgr.declareResource(path.c_str(), "CollisionObject",
                              resourceGroup, &nifCollisionLoader);
  } else if (ext == "dds"sv) {
    resGrpMgr.declareResource(path.c_str(), "Texture", resourceGroup);
  } else if (ext == "xml"sv || ext == "txt"sv) {
    resGrpMgr.declareResource(path.c_str(), "Text", resourceGroup);
  }
}

void Application::declareBsaArchive(const fs::Path &bsaFilename) {
  auto &resGrpMgr{Ogre::ResourceGroupManager::getSingleton()};
  const auto sysPath{bsaFilename.sysPath()};
  resGrpMgr.addResourceLocation(sysPath, "BSA", settings::resourceGroup);
}

void Application::declareBsaResources(const fs::Path &bsaFilename) {
  auto &archiveMgr{Ogre::ArchiveManager::getSingleton()};
  const auto sysPath{bsaFilename.sysPath()};
  const Ogre::Archive *archive{archiveMgr.load(sysPath, "BSA", true)};
  const Ogre::StringVectorPtr files{archive->list()};

  for (const auto &filename : *files) {
    declareResource(fs::Path{filename}, settings::resourceGroup);
  }
}

void Application::pollEvents() {
  const auto &settings{GameSettings::getSingleton()};
  const float sensitivity{settings.fGet("Controls.fMouseSensitivity")};
  sdl::Event sdlEvent;
  while (sdl::pollEvent(sdlEvent)) {

    imguiMgr->handleEvent(sdlEvent);

    auto keyEvent{keyMap->translateKey(sdlEvent)};
    if (keyEvent) {
      std::visit(overloaded{
          [this](event::Forward e) { playerController->handleEvent(e); },
          [this](event::Backward e) { playerController->handleEvent(e); },
          [this](event::SlideLeft e) { playerController->handleEvent(e); },
          [this](event::SlideRight e) { playerController->handleEvent(e); },
          [](event::Use) {},
          [](event::Activate) {},
          [](event::Block) {},
          [](event::Cast) {},
          [](event::ReadyItem) {},
          [this](event::Sneak e) { playerController->handleEvent(e); },
          [this](event::Run e) { playerController->handleEvent(e); },
          [this](event::AlwaysRun e) { playerController->handleEvent(e); },
          [](event::AutoMove) {},
          [this](event::Jump e) { playerController->handleEvent(e); },
          [](event::TogglePov) {},
          [](event::MenuMode) {},
          [](event::Rest) {},
          [](event::QuickMenu) {},
          [](event::Quick) {},
          [](event::QuickSave) {},
          [](event::QuickLoad) {},
          [](event::Grab) {}
      }, *keyEvent);
      continue;
    }

    switch (sdl::typeOf(sdlEvent)) {
      case sdl::EventType::Quit: {
        ogreRoot->queueEndRendering();
        break;
      }
      case sdl::EventType::KeyDown: {
        if (sdlEvent.key.repeat) break;
        switch (sdl::keyCodeOf(sdlEvent.key)) {
          case sdl::KeyCode::H: {
            drawBulletDebug = !drawBulletDebug;
            enableBulletDebugDraw(drawBulletDebug);
            break;
          }
          case sdl::KeyCode::Escape: {
            ogreRoot->queueEndRendering();
            break;
          }
          default:break;
        }
        break;
      }
      case sdl::EventType::MouseMotion: {
        const event::Pitch pitch{{sdlEvent.motion.yrel * sensitivity}};
        const event::Yaw yaw{{sdlEvent.motion.xrel * sensitivity}};
        playerController->handleEvent(pitch);
        playerController->handleEvent(yaw);
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
  auto *const btDispatcher{currentCell->physicsWorld->getDispatcher()};
  gsl::not_null dispatcher{dynamic_cast<btCollisionDispatcher *>(btDispatcher)};
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

RefId Application::getCrosshairRef() {
  using namespace Ogre::conversions;
  using namespace engine::conversions;
  GameSetting<int> iActivatePickLength{"iActivatePickLength", 150};

  auto *const camera{playerController->getCamera()};
  const auto cameraPos{toBullet(camera->getDerivedPosition())};
  const auto cameraDir{toBullet(camera->getDerivedDirection())};
  const auto rayStart{cameraPos + 0.5f * cameraDir};
  const float rayLength{*iActivatePickLength * metersPerUnit<float>};
  const auto rayEnd{cameraPos + rayLength * cameraDir};

  btCollisionWorld::ClosestRayResultCallback callback(rayStart, rayEnd);
  currentCell->physicsWorld->rayTest(rayStart, rayEnd, callback);

  if (callback.hasHit()) {
    return RefId{decodeFormId(callback.m_collisionObject->getUserPointer())};
  } else {
    return RefId{};
  }
}

bool Application::frameStarted(const Ogre::FrameEvent &event) {
  imguiMgr->newFrame(event.timeSinceLastFrame);
  //bool showDemoWindow{true};
  //ImGui::ShowDemoWindow(&showDemoWindow);

  pollEvents();
  playerController->update(event.timeSinceLastFrame);
  currentCell->physicsWorld->stepSimulation(event.timeSinceLastFrame);
  dispatchCollisions();

  static RefId refUnderCrosshair{0};
  RefId newRefUnderCrosshair = getCrosshairRef();
  if (newRefUnderCrosshair != refUnderCrosshair) {
    refUnderCrosshair = newRefUnderCrosshair;
    logger->info("Looking at 0x{:x}", static_cast<FormId>(refUnderCrosshair));
  }

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
