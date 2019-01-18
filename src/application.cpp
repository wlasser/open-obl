#include "application.hpp"
#include "bullet/collision.hpp"
#include "conversions.hpp"
#include "console_functions.hpp"
#include "keep_strategy.hpp"
#include "settings.hpp"
#include "esp.hpp"
#include "esp_coordinator.hpp"
#include "fs/path.hpp"
#include "game_settings.hpp"
#include "gui/logging.hpp"
#include "gui/menu.hpp"
#include "initial_record_visitor.hpp"
#include "meta.hpp"
#include "ogre/ogre_stream_wrappers.hpp"
#include "ogre/spdlog_listener.hpp"
#include "ogre/window.hpp"
#include "ogrebullet/conversions.hpp"
#include "ogreimgui/imgui_manager.hpp"
#include "resolvers/static_resolver.hpp"
#include "sdl/sdl.hpp"
#include <boost/algorithm/string.hpp>
#include <OgreOverlaySystem.h>
#include <OgreOverlayManager.h>
#include <OgreRenderQueue.h>
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

namespace oo {

Application::Application(std::string windowName) : FrameListener() {
  createLoggers();
  ctx.logger = spdlog::get(oo::LOG);

  loadIniConfiguration();
  auto &gameSettings = oo::GameSettings::getSingleton();

  auto &&[ogreRoot, overlaySys] = createOgreRoot();
  ctx.ogreRoot = std::move(ogreRoot);
  ctx.overlaySys = std::move(overlaySys);

  // Creating an Ogre::RenderWindow initialises the render system, which is
  // necessary to create shaders (including reading scripts), so this has to be
  // done early.
  ctx.sdlInit = std::make_unique<sdl::Init>();
  ctx.windows = createWindow(windowName);

  // Set the keyboard configuration
  ctx.keyMap = std::make_unique<oo::event::KeyMap>(gameSettings);

  // Construct the Bullet configuration
  ctx.bulletConf = std::make_unique<bullet::Configuration>();

  // Start the sound engine
  ctx.soundMgr = std::make_unique<Ogre::SoundManager>();
  setSoundSettings();

  // Start the developer console backend
  ctx.consoleEngine = std::make_unique<oo::ConsoleEngine>();
  registerConsoleFunctions();

  // Add the resource managers
  ctx.nifResourceMgr = std::make_unique<Ogre::NifResourceManager>();
  ctx.collisionObjectMgr = std::make_unique<Ogre::CollisionObjectManager>();
  ctx.textResourceMgr = std::make_unique<Ogre::TextResourceManager>();
  ctx.wavResourceMgr = std::make_unique<Ogre::WavResourceManager>();

  // Add the factories
  ctx.rigidBodyFactory = std::make_unique<Ogre::RigidBodyFactory>();
  ctx.ogreRoot->addMovableObjectFactory(ctx.rigidBodyFactory.get());

  // Add the codecs
  ctx.texImageCodec = std::make_unique<Ogre::TexImageCodec>();
  Ogre::Codec::registerCodec(ctx.texImageCodec.get());

  // Create the engine managers
  ctx.doorRes = std::make_unique<oo::DoorResolver>();
  ctx.lighRes = std::make_unique<oo::LighResolver>();
  ctx.statRes = std::make_unique<oo::StatResolver>();
  ctx.actiRes = std::make_unique<oo::ActiResolver>();
  ctx.refrDoorRes = std::make_unique<oo::RefrDoorResolver>();
  ctx.refrLighRes = std::make_unique<oo::RefrLighResolver>();
  ctx.refrStatRes = std::make_unique<oo::RefrStatResolver>();
  ctx.refrActiRes = std::make_unique<oo::RefrActiResolver>();

  ctx.cellRes = std::make_unique<oo::CellResolver>(*ctx.bulletConf);

  // Add the main resource group
  auto &resGrpMgr = Ogre::ResourceGroupManager::getSingleton();
  resGrpMgr.createResourceGroup(oo::RESOURCE_GROUP);

  // Shaders are not stored in the data folder (mostly for vcs reasons)
  resGrpMgr.addResourceLocation("./shaders", "FileSystem",
                                oo::SHADER_GROUP);

  // Register the BSA archive format
  auto &archiveMgr = Ogre::ArchiveManager::getSingleton();
  ctx.bsaArchiveFactory = std::make_unique<Ogre::BsaArchiveFactory>();
  archiveMgr.addArchiveFactory(ctx.bsaArchiveFactory.get());

  // Grab the data folder from the ini file
  const oo::Path dataPath{gameSettings.get("General.SLocalMasterPath", "Data")};

  // Loading from the filesystem is favoured over bsa files so that mods can
  // replace data. Marking the folder as recursive means that files are
  // identified by their full path.
  // The FileSystem archive has platform-dependent behaviour, and in particular
  // is case-sensitive on *nix, but we need case-insensitivity on all platforms.
  // Files must therefore have lowercase names to correctly override.
  // TODO: Replace FileSystem with a case-insensitive version
  resGrpMgr.addResourceLocation(dataPath.c_str(),
                                "FileSystem",
                                oo::RESOURCE_GROUP,
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
  declareFilesystemResources(dataPath);

  // All resources have been declared by now, so we can initialise the resource
  // groups. This won't initialise the default groups.
  resGrpMgr.initialiseResourceGroup(oo::RESOURCE_GROUP, false);
  resGrpMgr.initialiseResourceGroup(oo::SHADER_GROUP, true);

  ctx.imguiMgr = std::make_unique<Ogre::ImGuiManager>();

  // Load esp files
  const auto loadOrder{getLoadOrder(dataPath)};
  ctx.logger->info("Mod load order:");
  for (int i = 0; i < static_cast<int>(loadOrder.size()); ++i) {
    ctx.logger->info("0x{:0>2x} {}", i, loadOrder[i].view());
  }
  ctx.espCoordinator = std::make_unique<oo::EspCoordinator>(loadOrder.begin(),
                                                            loadOrder.end());
  // Read the main esm
  InitialRecordVisitor initialRecordVisitor(ctx.doorRes.get(),
                                            ctx.lighRes.get(),
                                            ctx.statRes.get(),
                                            ctx.actiRes.get(),
                                            ctx.cellRes.get());
  for (int i = 0; i < static_cast<int>(loadOrder.size()); ++i) {
    oo::readEsp(*ctx.espCoordinator, i, initialRecordVisitor);
  }

  createDummySceneManager();
  createDummyRenderQueue();

  modeStack.emplace_back(std::in_place_type<oo::MainMenuMode>, ctx);
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
  auto ogreLogger{std::make_shared<spdlog::logger>(oo::OGRE_LOG, sinks)};
  spdlog::register_logger(ogreLogger);
  auto *defaultLog{ogreLogMgr.createLog("Default", true, true, true)};
  ogreLogListener = std::make_unique<Ogre::SpdlogListener>(oo::OGRE_LOG);
  defaultLog->addListener(ogreLogListener.get());

  // Construct our own logger
  auto logger{std::make_shared<spdlog::logger>(oo::LOG, sinks)};
  spdlog::register_logger(logger);

  // Set the starting logger levels. These will be modified later according to
  // user settings, if specified.
  logger->set_level(spdlog::level::warn);
  ogreLogger->set_level(spdlog::level::info);

  // Set the library loggers
  gui::guiLogger(oo::LOG);
}

void Application::loadIniConfiguration() {
  auto &gameSettings{GameSettings::getSingleton()};
  auto logger{spdlog::get(oo::LOG)};

  logger->info("Parsing {}", oo::DEFAULT_INI);
  gameSettings.load(oo::DEFAULT_INI, true);

  if (std::filesystem::is_regular_file(oo::USER_INI)) {
    logger->info("Parsing {}", oo::USER_INI);
    gameSettings.load(oo::USER_INI, true);
  } else {
    logger->warn("User configuration {} not found", oo::USER_INI);
  }

  // Override the logger levels with user-provided ones, if any
  if (const auto level = gameSettings.get<std::string>("Debug.sOgreLogLevel")) {
    spdlog::get(oo::OGRE_LOG)->set_level(spdlog::level::from_str(*level));
  }
  if (const auto level = gameSettings.get<std::string>("Debug.sLogLevel")) {
    spdlog::get(oo::LOG)->set_level(spdlog::level::from_str(*level));
  }
}

void Application::setRenderSystem(Ogre::Root *root,
                                  const std::string &systemName) {
  if (auto *renderSystem = root->getRenderSystemByName(systemName)) {
    root->setRenderSystem(renderSystem);
  } else {
    // List the available render systems
    auto logger{spdlog::get(oo::LOG)};
    logger->error("Render system {} not found", systemName);
    logger->info("Available render systems are:");
    for (const auto &system : root->getAvailableRenderers()) {
      logger->info(" * {}", system->getName());
    }
    throw std::runtime_error("Invalid render system");
  }
}

std::tuple<std::unique_ptr<Ogre::Root>, std::unique_ptr<Ogre::OverlaySystem>>
Application::createOgreRoot() {
  auto root{std::make_unique<Ogre::Root>("plugins.cfg", "", "")};
  auto overlaySys{std::make_unique<Ogre::OverlaySystem>()};
  setRenderSystem(root.get(), "OpenGL 3+ Rendering Subsystem");
  root->initialise(false);
  root->addFrameListener(this);
  return {std::move(root), std::move(overlaySys)};
}

std::tuple<sdl::WindowPtr, Ogre::RenderWindowPtr>
Application::createWindow(const std::string &windowName) {
  const auto &gameSettings{oo::GameSettings::getSingleton()};
  auto logger{spdlog::get(oo::LOG)};

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

void Application::setSoundSettings() {
  const auto &gameSettings{oo::GameSettings::getSingleton()};

  auto masterBus{ctx.soundMgr->getMasterBus()};
  masterBus.setVolume(gameSettings.get("Audio.fDefaultMasterVolume", 1.0f));

  auto musicBus{ctx.soundMgr->getMusicBus()};
  musicBus.setVolume(gameSettings.get("Audio.fDefaultMusicVolume", 1.0f));

  auto effectBus{ctx.soundMgr->createMixingBus("effect")};
  effectBus.setVolume(gameSettings.get("Audio.fDefaultEffectsVolume", 1.0f));

  auto footBus{ctx.soundMgr->createMixingBus("foot")};
  footBus.setVolume(gameSettings.get("Audio.fDefaultFootVolume", 1.0f));

  auto voiceBus{ctx.soundMgr->createMixingBus("voice")};
  voiceBus.setVolume(gameSettings.get("Audio.fDefaultVoiceVolume", 1.0f));
}

std::vector<oo::Path>
Application::parseBsaList(const oo::Path &masterPath, const std::string &list) {
  std::vector<std::string> names{};

  // Split them on commas, this leaves trailing whitespace
  boost::split(names, list, [](char c) { return c == ','; });

  // Trim the whitespace and append the data folder
  std::vector<oo::Path> filenames(names.size());
  std::transform(names.begin(), names.end(), filenames.begin(),
                 [&masterPath](std::string name) {
                   boost::trim(name);
                   return masterPath / oo::Path{name};
                 });

  // Reject any invalid ones
  const auto predicate = [](const auto &filename) {
    return !filename.exists();
  };
  filenames.erase(std::remove_if(filenames.begin(), filenames.end(), predicate),
                  filenames.end());

  return filenames;
}

void Application::declareResource(const oo::Path &path,
                                  const std::string &resourceGroup) {
  using namespace std::literals;
  auto &resGrpMgr{Ogre::ResourceGroupManager::getSingleton()};
  const auto ext{path.extension()};

  if (ext == "nif"sv) {
    resGrpMgr.declareResource(path.c_str(), "Nif", resourceGroup);
    resGrpMgr.declareResource(path.c_str(), "Mesh",
                              resourceGroup, &ctx.nifLoader);
    resGrpMgr.declareResource(path.c_str(), "CollisionObject",
                              resourceGroup, &ctx.nifCollisionLoader);
    // TODO: Do all skeletons end with "skeleton.nif"?
    resGrpMgr.declareResource(path.c_str(), "Skeleton",
                              resourceGroup, &ctx.skeletonLoader);
  } else if (ext == "dds"sv) {
    resGrpMgr.declareResource(path.c_str(), "Texture", resourceGroup);
  } else if (ext == "xml"sv || ext == "txt"sv) {
    resGrpMgr.declareResource(path.c_str(), "Text", resourceGroup);
  } else if (ext == "ttf"sv) {
    resGrpMgr.declareResource(path.c_str(), "Font", resourceGroup);
  } else if (ext == "fnt"sv) {
    resGrpMgr.declareResource(path.c_str(), "Font",
                              resourceGroup, &ctx.fntLoader);
  } else if (ext == "tex"sv) {
    resGrpMgr.declareResource(path.c_str(), "Texture", resourceGroup);
  } else if (ext == "wav"sv || ext == "mp3") {
    resGrpMgr.declareResource(path.c_str(), "Wav", resourceGroup);
  }
}

void Application::declareBsaArchive(const oo::Path &bsaFilename) {
  auto &resGrpMgr{Ogre::ResourceGroupManager::getSingleton()};
  const auto sysPath{bsaFilename.sysPath()};
  resGrpMgr.addResourceLocation(sysPath, "BSA", oo::RESOURCE_GROUP);
}

void Application::declareBsaResources(const oo::Path &bsaFilename) {
  auto &archiveMgr{Ogre::ArchiveManager::getSingleton()};
  const auto sysPath{bsaFilename.sysPath()};
  const Ogre::Archive *archive{archiveMgr.load(sysPath, "BSA", true)};
  const Ogre::StringVectorPtr files{archive->list()};

  for (auto &filename : *files) {
    declareResource(oo::Path{std::move(filename)}, oo::RESOURCE_GROUP);
  }
}

void Application::declareFilesystemResources(const oo::Path &foldername) {
  auto &archiveMgr{Ogre::ArchiveManager::getSingleton()};
  const auto sysPath{foldername.sysPath()};
  const Ogre::Archive *archive{archiveMgr.load(sysPath, "FileSystem", true)};
  const Ogre::StringVectorPtr files{archive->list()};

  auto &resGrpMgr{Ogre::ResourceGroupManager::getSingleton()};

  // Cannot use resourceExists since it doesn't check for declaration, just the
  // existence of a file with the given name in some location.
  auto resDecls{resGrpMgr.getResourceDeclarationList(oo::RESOURCE_GROUP)};

  // This is like O(n^2) with the number of resources on a heavily modded game,
  // which is kind of sad.
  for (auto &filename : *files) {
    oo::Path path{std::move(filename)};
    std::string pathString{path.c_str()};
    if (std::find_if(resDecls.begin(), resDecls.end(), [&](const auto &decl) {
      return decl.resourceName == pathString;
    }) == resDecls.end()) {
      declareResource(path, oo::RESOURCE_GROUP);
    }
  }
}

std::vector<oo::Path> Application::getLoadOrder(const oo::Path &masterPath) {
  // TODO: Use an oo::DirectoryIterator for case-insensitivity
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

  std::vector<oo::Path> out(files.size());
  std::transform(files.begin(), files.end(), out.begin(), [](const auto &e) {
    return oo::Path{std::string{e.path()}};
  });

  return out;
}

void Application::registerConsoleFunctions() {
  auto rcf = [this](const std::string &name, auto F) {
    using F_t = std::remove_pointer_t<decltype(F)>;
    ctx.consoleEngine->registerFunction<F_t>(name);
  };

  rcf("QuitGame", &console::QuitGame);
  rcf("qqq", &console::qqq);
  rcf("ToggleCollisionGeometry", &console::ToggleCollisionGeometry);
  rcf("tcg", &console::tcg);
  rcf("ShowMainMenu", &console::ShowMainMenu);
  rcf("ShowClassMenu", &console::ShowClassMenu);
  rcf("ShowEnchantmentMenu", &console::ShowEnchantmentMenu);
  rcf("ShowMap", &console::ShowMap);
  rcf("ShowRaceMenu", &console::ShowRaceMenu);
  rcf("ShowSpellmaking", &console::ShowSpellmaking);
}

void Application::pollEvents() {
  sdl::Event sdlEvent;
  while (sdl::pollEvent(sdlEvent)) {
    if (sdl::typeOf(sdlEvent) == sdl::EventType::Quit) {
      quit();
      return;
    }

    // Pass event to ImGui and let ImGui consume it if it wants
    ctx.imguiMgr->handleEvent(sdlEvent);
    auto imguiIo{ImGui::GetIO()};
    if (imguiIo.WantCaptureKeyboard && sdl::isKeyboardEvent(sdlEvent)) continue;
    else if (imguiIo.WantCaptureMouse && sdl::isMouseEvent(sdlEvent)) continue;

    std::visit([this, sdlEvent](auto &mode) {
      auto[pop, push]{mode.handleEvent(ctx, sdlEvent)};
      if (pop) popMode();
      if (push) {
        pushMode(std::move(*push));
      } else {
        refocusMode();
      }
    }, modeStack.back());
  }
}

void Application::createDummySceneManager() {
  dummyScnMgr = ctx.getRoot().createSceneManager("DefaultSceneManager",
                                                 "__DummySceneManager");
  dummyCamera = dummyScnMgr->createCamera("__DummyCamera");
  dummyScnMgr->addRenderQueueListener(ctx.getImGuiManager());
  dummyScnMgr->addRenderQueueListener(ctx.getOverlaySystem());
  ctx.setCamera(gsl::make_not_null(dummyCamera));
}

void Application::createDummyRenderQueue() {
  Ogre::RenderQueue dummyQueue{};
  Ogre::OverlayManager::getSingleton()._queueOverlaysForRendering(
      dummyCamera, &dummyQueue, dummyCamera->getViewport());
}

void Application::quit() {
  ctx.ogreRoot->queueEndRendering();
}

bool Application::isGameMode() const {
  if (modeStack.empty()) return false;
  return std::holds_alternative<oo::GameMode>(modeStack.back());
}

bool Application::isConsoleMode() const {
  if (modeStack.empty()) return false;
  return std::holds_alternative<oo::ConsoleMode>(modeStack.back());
}

oo::GameMode &Application::getGameMode() {
  return std::get<oo::GameMode>(modeStack.back());
}

bool Application::isGameModeInStack() const {
  return std::any_of(modeStack.rbegin(), modeStack.rend(), [](const auto &m) {
    return std::holds_alternative<oo::GameMode>(m);
  });
}

oo::GameMode &Application::getGameModeInStack() {
  auto it{std::find_if(modeStack.rbegin(), modeStack.rend(), [](const auto &m) {
    return std::holds_alternative<oo::GameMode>(m);
  })};
  return std::get<oo::GameMode>(*it);
}

bool Application::frameStarted(const Ogre::FrameEvent &event) {
  pollEvents();
  if (modeStack.empty()) {
    quit();
    return false;
  }

  ctx.imguiMgr->newFrame(event.timeSinceLastFrame);

  std::visit([this, &event](auto &mode) {
    mode.update(ctx, event.timeSinceLastFrame);
  }, modeStack.back());

  if (deferredMode) {
    if (isConsoleMode()) popMode();
    pushMode(std::move(*deferredMode));
    deferredMode.reset();
  }

  return true;
}

bool Application::frameRenderingQueued(const Ogre::FrameEvent &/*event*/) {
  return true;
}

bool Application::frameEnded(const Ogre::FrameEvent &/*event*/) {
  return true;
}

} // namespace oo
