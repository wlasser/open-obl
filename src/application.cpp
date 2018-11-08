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

  ctx.ogreRoot = createOgreRoot();

  // Creating an Ogre::RenderWindow initialises the render system, which is
  // necessary to create shaders (including reading scripts), so this has to be
  // done early.
  ctx.sdlInit = std::make_unique<sdl::Init>();
  ctx.windows = createWindow(windowName);

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

  // Create the engine managers
  ctx.doorRes = std::make_unique<DoorResolver>();
  ctx.lightRes = std::make_unique<LightResolver>();
  ctx.staticRes = std::make_unique<StaticResolver>();
  ctx.interiorCellRes = std::make_unique<InteriorCellResolver>(
      InteriorCellResolver::resolvers_t{
          *ctx.doorRes, *ctx.lightRes, *ctx.staticRes
      },
      *ctx.bulletConf,
      std::make_unique<strategy::KeepCurrent<InteriorCell>>());

  // Add the main resource group
  auto &resGrpMgr = Ogre::ResourceGroupManager::getSingleton();
  resGrpMgr.createResourceGroup(settings::resourceGroup);

  // Shaders are not stored in the data folder (mostly for vcs reasons)
  resGrpMgr.addResourceLocation("./shaders", "FileSystem",
                                settings::resourceGroup);

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
  resGrpMgr.addResourceLocation(dataPath.c_str(),
                                "FileSystem",
                                settings::resourceGroup,
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

  // All resources have been declared by now, so we can initialise the resource
  // groups. This won't initialise the default groups.
  resGrpMgr.initialiseAllResourceGroups();

  ctx.imguiMgr = std::make_unique<Ogre::ImGuiManager>();

  // Load esp files
  const auto loadOrder{getLoadOrder(dataPath)};
  ctx.logger->info("Mod load order:");
  for (int i = 0; i < loadOrder.size(); ++i) {
    ctx.logger->info("0x{:0>2x} {}", i, loadOrder[i].view());
  }
  ctx.espCoordinator = std::make_unique<esp::EspCoordinator>(loadOrder.begin(),
                                                             loadOrder.end());
  // Read the main esm
  InitialProcessor initialProcessor(ctx.doorRes.get(),
                                    ctx.lightRes.get(),
                                    ctx.staticRes.get(),
                                    ctx.interiorCellRes.get());
  for (int i = 0; i < loadOrder.size(); ++i) {
    esp::readEsp(*ctx.espCoordinator, i, initialProcessor);
  }

  modeStack.emplace_back(std::in_place_type<GameMode>, ctx);
  std::visit([this](auto &&state) {
    state.enter(ctx);
  }, modeStack.back());
}

void Application::createLoggers() {
  using spdlog::sinks::stdout_color_sink_mt;
  using spdlog::sinks::basic_file_sink_mt;

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
  auto *defaultLog{ogreLogMgr.createLog("Default", true, true, true)};
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
  auto logger{spdlog::get(settings::log)};

  logger->info("Parsing {}", settings::defaultIni);
  gameSettings.load(settings::defaultIni, true);

  if (std::filesystem::is_regular_file(settings::userIni)) {
    logger->info("Parsing {}", settings::userIni);
    gameSettings.load(settings::userIni, true);
  } else {
    logger->warn("User configuration {} not found", settings::userIni);
  }

  // Override the logger levels with user-provided ones, if any
  if (const auto level = gameSettings.get<std::string>("Debug.sOgreLogLevel")) {
    spdlog::get(settings::ogreLog)->set_level(spdlog::level::from_str(*level));
  }
  if (const auto level = gameSettings.get<std::string>("Debug.sLogLevel")) {
    spdlog::get(settings::log)->set_level(spdlog::level::from_str(*level));
  }
}

void Application::setRenderSystem(Ogre::Root *root,
                                  const std::string &systemName) {
  if (auto *renderSystem = root->getRenderSystemByName(systemName)) {
    root->setRenderSystem(renderSystem);
  } else {
    // List the available render systems
    auto logger{spdlog::get(settings::log)};
    logger->error("Render system {} not found", systemName);
    logger->info("Available render systems are:");
    for (const auto &system : root->getAvailableRenderers()) {
      logger->info(" * {}", system->getName());
    }
    throw std::runtime_error("Invalid render system");
  }
}

std::unique_ptr<Ogre::Root> Application::createOgreRoot() {
  auto root{std::make_unique<Ogre::Root>("plugins.cfg", "", "")};
  setRenderSystem(root.get(), "OpenGL 3+ Rendering Subsystem");
  root->initialise(false);
  root->addFrameListener(this);
  return root;
}

std::tuple<sdl::WindowPtr, Ogre::RenderWindowPtr>
Application::createWindow(const std::string &windowName) {
  const auto &gameSettings{GameSettings::getSingleton()};
  auto logger{spdlog::get(settings::log)};

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
  auto sdlWindow{sdl::makeWindow(windowName, windowWidth, windowHeight,
                                 windowFlags)};
  const auto sdlWindowInfo{sdl::getSysWMInfo(sdlWindow.get())};
  const auto parent{sdl::getWindowParent(sdlWindowInfo)};

  // Construct a render window with the SDL window as a parent; SDL handles the
  // window itself, Ogre manages the OpenGL context.
  const std::map<std::string, std::string> params{
      {"parentWindowHandle", parent}
  };
  auto ogreWindow{Ogre::makeRenderWindow(
      Ogre::Root::getSingletonPtr(),
      windowName,
      static_cast<const unsigned>(windowWidth),
      static_cast<const unsigned>(windowHeight),
      &params)};

  return {std::move(sdlWindow), std::move(ogreWindow)};
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
    if (sdl::typeOf(sdlEvent) == sdl::EventType::Quit) {
      ctx.ogreRoot->queueEndRendering();
      return;
    }

    // Pass event to ImGui and let ImGui consume it if it wants
    ctx.imguiMgr->handleEvent(sdlEvent);
    auto imguiIo{ImGui::GetIO()};
    if (imguiIo.WantCaptureKeyboard && isKeyboardEvent(sdlEvent)) continue;
    else if (imguiIo.WantCaptureMouse && isMouseEvent(sdlEvent)) continue;

    std::visit([this, sdlEvent](auto &mode) {
      auto[pop, push]{mode.handleEvent(ctx, sdlEvent)};
      if (pop) popMode();
      if (push) {
        pushMode(*push);
      } else {
        refocusMode();
      }
    }, modeStack.back());
  }
}

bool Application::isKeyboardEvent(const sdl::Event &e) noexcept {
  const auto type{sdl::typeOf(e)};
  return type == sdl::EventType::KeyUp
      || type == sdl::EventType::KeyDown
      || type == sdl::EventType::TextInput
      || type == sdl::EventType::TextEditing;
}

bool Application::isMouseEvent(const sdl::Event &e) noexcept {
  const auto type{sdl::typeOf(e)};
  return type == sdl::EventType::MouseMotion
      || type == sdl::EventType::MouseButtonDown
      || type == sdl::EventType::MouseButtonUp
      || type == sdl::EventType::MouseWheel;
}

bool Application::frameStarted(const Ogre::FrameEvent &event) {
  pollEvents();

  ctx.imguiMgr->newFrame(event.timeSinceLastFrame);

  std::visit([this, &event](auto &mode) {
    mode.update(ctx, event.timeSinceLastFrame);
  }, modeStack.back());

  return true;
}

bool Application::frameRenderingQueued(const Ogre::FrameEvent &event) {
  return true;
}

bool Application::frameEnded(const Ogre::FrameEvent &event) {
  return true;
}
