#include "application.hpp"
#include "bullet/collision.hpp"
#include "conversions.hpp"
#include "keep_strategy.hpp"
#include "settings.hpp"
#include "esp.hpp"
#include "esp_coordinator.hpp"
#include "fs/path.hpp"
#include "game_settings.hpp"
#include "initial_processor.hpp"
#include "meta.hpp"
#include "ogre/ogre_stream_wrappers.hpp"
#include "ogre/spdlog_listener.hpp"
#include "ogre/window.hpp"
#include "ogrebullet/conversions.hpp"
#include "ogreimgui/imgui_manager.hpp"
#include "resolvers/interior_cell_resolver.hpp"
#include "resolvers/static_resolver.hpp"
#include "sdl/sdl.hpp"
#include <boost/algorithm/string.hpp>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

Application::Application(std::string windowName) : FrameListener() {
  createLoggers();
  ctx.logger = spdlog::get(settings::log);

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
  ctx.ogreRoot = std::make_unique<Ogre::Root>("plugins.cfg", "", "");
  setRenderSystem("OpenGL 3+ Rendering Subsystem");
  ctx.ogreRoot->initialise(false);
  ctx.ogreRoot->addFrameListener(this);

  // Create the window
  ctx.sdlInit = sdl::Init();
  createWindow(windowName);

  // Set the keyboard configuration
  ctx.keyMap = std::make_unique<KeyMap>(gameSettings);

  // Construct the Bullet configuration
  ctx.bulletConf = std::make_unique<bullet::Configuration>();

  // Add the resource managers
  ctx.collisionObjectMgr = std::make_unique<Ogre::CollisionObjectManager>();
  ctx.textResourceMgr = std::make_unique<Ogre::TextResourceManager>();

  // Add the factories
  ctx.rigidBodyFactory = std::make_unique<Ogre::RigidBodyFactory>();
  ctx.ogreRoot->addMovableObjectFactory(ctx.rigidBodyFactory.get());

  // Add the main resource group
  const std::string resourceGroup{settings::resourceGroup};
  auto &resGrpMgr = Ogre::ResourceGroupManager::getSingleton();
  resGrpMgr.createResourceGroup(resourceGroup);

  // Register the BSA archive format
  auto &archiveMgr = Ogre::ArchiveManager::getSingleton();
  ctx.bsaArchiveFactory = std::make_unique<Ogre::BsaArchiveFactory>();
  archiveMgr.addArchiveFactory(ctx.bsaArchiveFactory.get());

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

  // Load esp files
  const auto loadOrder{getLoadOrder(dataPath)};
  ctx.logger->info("Mod load order:");
  for (int i = 0; i < loadOrder.size(); ++i) {
    ctx.logger->info("0x{:0>2x} {}", i, loadOrder[i].view());
  }
  ctx.espCoordinator = std::make_unique<esp::EspCoordinator>(loadOrder.begin(),
                                                             loadOrder.end());

  // Create the engine managers
  ctx.doorRes = std::make_unique<DoorResolver>();
  ctx.lightRes = std::make_unique<LightResolver>();
  ctx.staticRes = std::make_unique<StaticResolver>();
  InteriorCellResolver::resolvers_t resolvers{
      *ctx.doorRes, *ctx.lightRes, *ctx.staticRes
  };
  ctx.interiorCellRes = std::make_unique<InteriorCellResolver>(
      resolvers,
      *ctx.bulletConf,
      std::make_unique<strategy::KeepCurrent<InteriorCell>>());

  // Read the main esm
  InitialProcessor initialProcessor(ctx.doorRes.get(),
                                    ctx.lightRes.get(),
                                    ctx.staticRes.get(),
                                    ctx.interiorCellRes.get());
  for (int i = 0; i < loadOrder.size(); ++i) {
    esp::readEsp(*ctx.espCoordinator, i, initialProcessor);
  }

  ctx.imguiMgr = std::make_unique<Ogre::ImGuiManager>();

  modeStack.emplace_back(std::in_place_type<GameMode>, ctx);
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
  ctx.ogreLogMgr = std::make_unique<Ogre::LogManager>();
  auto *defaultLog{ctx.ogreLogMgr->createLog("Default", true, true, true)};
  ctx.ogreLogListener =
      std::make_unique<Ogre::SpdlogListener>(settings::ogreLog);
  defaultLog->addListener(ctx.ogreLogListener.get());

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

  ctx.logger->info("Parsing {}", settings::defaultIni);
  gameSettings.load(settings::defaultIni, true);

  if (std::filesystem::is_regular_file(settings::userIni)) {
    ctx.logger->info("Parsing {}", settings::userIni);
    gameSettings.load(settings::userIni, true);
  } else {
    ctx.logger->warn("User configuration {} not found", settings::userIni);
  }
}

void Application::setRenderSystem(const std::string &systemName) {
  auto &root = Ogre::Root::getSingleton();
  if (auto *renderSystem = root.getRenderSystemByName(systemName)) {
    root.setRenderSystem(renderSystem);
  } else {
    // List the available render systems
    ctx.logger->error("Render system {} not found", systemName);
    ctx.logger->info("Available render systems are:");
    for (const auto &system : root.getAvailableRenderers()) {
      ctx.logger->info(" * {}", system->getName());
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
    ctx.logger->critical("Cannot create a window with width {} and height {}",
                         windowWidth, windowHeight);
    ctx.logger->critical(
        "Set 'Display.iSize W' and 'Display.iSize H' to sensible values");
    throw std::runtime_error("Cannot create window with negative size");
  }

  // Grab the window settings
  sdl::WindowFlags windowFlags{};
  if (gameSettings.bGet("Display.bFull Screen")) {
    windowFlags |= sdl::WindowFlags::Fullscreen;
  }

  // Make the window and find its system parent handle
  ctx.sdlWindow = sdl::makeWindow(windowName, windowWidth, windowHeight,
                                  windowFlags);
  const auto sdlWindowInfo{sdl::getSysWMInfo(ctx.sdlWindow.get())};
  const auto parent{sdl::getWindowParent(sdlWindowInfo)};

  // Make cursor behaviour more sensible
  //sdl::setRelativeMouseMode(true);

  // Construct a render window with the SDL window as a parent; SDL handles the
  // window itself, Ogre manages the OpenGL context.
  const std::map<std::string, std::string> params{
      {"parentWindowHandle", parent}
  };
  ctx.ogreWindow = Ogre::makeRenderWindow(
      ctx.ogreRoot.get(),
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
                              resourceGroup, &ctx.nifLoader);
    resGrpMgr.declareResource(path.c_str(), "CollisionObject",
                              resourceGroup, &ctx.nifCollisionLoader);
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

std::vector<fs::Path> Application::getLoadOrder(const fs::Path &masterPath) {
  // TODO: Use an fs::DirectoryIterator for case-insensitivity
  const auto sysPath{masterPath.sysPath()};
  std::vector<std::filesystem::directory_entry> files{};
  std::filesystem::directory_iterator dirIt{sysPath};
  auto espFilter = [](const auto &entry) {
    const std::filesystem::path ext{entry.path().extension()};
    return entry.is_regular_file() && (ext == ".esp" || ext == ".esm");
  };

  // C++20: Ranges
  std::copy_if(begin(dirIt), end(dirIt), std::back_inserter(files), espFilter);
  std::sort(files.begin(), files.end(), [](const auto &a, const auto &b) {
    return a.last_write_time() > b.last_write_time();
  });
  std::stable_partition(files.begin(), files.end(), [](const auto &entry) {
    return entry.path().extension() == ".esm";
  });

  std::vector<fs::Path> out(files.size());
  std::transform(files.begin(), files.end(), out.begin(), [](const auto &e) {
    return fs::Path{std::string{e.path()}};
  });

  return out;
}

void Application::pollEvents() {
  sdl::Event sdlEvent;
  while (sdl::pollEvent(sdlEvent)) {
    // Pass event to ImGui and let ImGui consume it if it wants
    ctx.imguiMgr->handleEvent(sdlEvent);
    auto imguiIo{ImGui::GetIO()};
    if (imguiIo.WantCaptureKeyboard && isKeyboardEvent(sdlEvent)) continue;
    else if (imguiIo.WantCaptureMouse && isMouseEvent(sdlEvent)) continue;

    std::visit(overloaded{
        [this, sdlEvent](GameMode &mode) { mode.handleEvent(ctx, sdlEvent); }
    }, modeStack.back());
  }
}

bool Application::isKeyboardEvent(const sdl::Event &e) const noexcept {
  const auto type{sdl::typeOf(e)};
  return type == sdl::EventType::KeyUp
      || type == sdl::EventType::KeyDown
      || type == sdl::EventType::TextInput
      || type == sdl::EventType::TextEditing;
}

bool Application::isMouseEvent(const sdl::Event &e) const noexcept {
  const auto type{sdl::typeOf(e)};
  return type == sdl::EventType::MouseMotion
      || type == sdl::EventType::MouseButtonDown
      || type == sdl::EventType::MouseButtonUp
      || type == sdl::EventType::MouseWheel;
}

bool Application::frameStarted(const Ogre::FrameEvent &event) {
  pollEvents();

  ctx.imguiMgr->newFrame(event.timeSinceLastFrame);
  bool showDemoWindow{true};
  ImGui::ShowDemoWindow(&showDemoWindow);

  std::visit(overloaded{
      [this, &event](GameMode &mode) {
        mode.update(ctx, event.timeSinceLastFrame);
      }
  }, modeStack.back());

  return true;
}

bool Application::frameRenderingQueued(const Ogre::FrameEvent &event) {
  return true;
}

bool Application::frameEnded(const Ogre::FrameEvent &event) {
  return true;
}
