#include "application.hpp"
#include "application_base.hpp"
#include "audio.hpp"
#include "bullet/collision.hpp"
#include "cell_cache.hpp"
#include "config/game_settings.hpp"
#include "console_functions.hpp"
#include "esp/esp.hpp"
#include "esp/esp_coordinator.hpp"
#include "fs/path.hpp"
#include "gui/logging.hpp"
#include "gui/menu.hpp"
#include "initial_record_visitor.hpp"
#include "job/job.hpp"
#include "math/conversions.hpp"
#include "mesh/entity.hpp"
#include "mesh/mesh_manager.hpp"
#include "mesh/subentity.hpp"
#include "nifloader/collision_object_loader.hpp"
#include "nifloader/logging.hpp"
#include "nifloader/mesh_loader.hpp"
#include "nifloader/nif_resource_manager.hpp"
#include "nifloader/skeleton_loader.hpp"
#include "ogre/deferred_light_pass.hpp"
#include "ogre/ogre_stream_wrappers.hpp"
#include "ogre/scene_manager.hpp"
#include "ogre/spdlog_listener.hpp"
#include "ogre/terrain_material_generator.hpp"
#include "ogre/window.hpp"
#include "ogrebullet/conversions.hpp"
#include "ogreimgui/imgui_manager.hpp"
#include "resolvers/acti_resolver.hpp"
#include "resolvers/cell_resolver.hpp"
#include "resolvers/cont_resolver.hpp"
#include "resolvers/door_resolver.hpp"
#include "resolvers/flor_resolver.hpp"
#include "resolvers/furn_resolver.hpp"
#include "resolvers/ligh_resolver.hpp"
#include "resolvers/npc__resolver.hpp"
#include "resolvers/stat_resolver.hpp"
#include "resolvers/wrld_resolver.hpp"
#include "script_functions.hpp"
#include "scripting/console_engine.hpp"
#include "scripting/script_engine.hpp"
#include "sdl/sdl.hpp"
#include "util/meta.hpp"
#include "util/settings.hpp"
#include <boost/algorithm/string.hpp>
#include <OgreOverlaySystem.h>
#include <OgreOverlayManager.h>
#include <OgreRenderQueue.h>

#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/basic_file_sink.h>
#if defined(_WIN32) || defined(_WIN64)
#include <spdlog/sinks/stdout_sinks.h>
#else
#include <spdlog/sinks/stdout_color_sinks.h>
#endif

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "util/windows_cleanup.hpp"

namespace oo {

Application::Application(std::string windowName) : FrameListener() {
  ogreLogListener = oo::createLoggers("OpenOBL.log", ogreLogMgr);
  gui::guiLogger(oo::LOG);
  oo::nifloaderLogger(oo::LOG);
  ctx.logger = spdlog::get(oo::LOG);

  oo::JobCounter iniCounter{1};
  oo::JobManager::runJob([]() { loadIniConfiguration(); }, &iniCounter);

  ctx.ogreRoot = std::make_unique<Ogre::Root>("", "", "");
  ctx.overlaySys = std::make_unique<Ogre::OverlaySystem>();
  ctx.gl3PlusPlugin = oo::startGl3Plus(ctx.ogreRoot.get());

  ctx.ogreRoot->initialise(false);
  ctx.ogreRoot->addFrameListener(this);

  oo::JobManager::waitOn(&iniCounter);
  auto &gameSettings = oo::GameSettings::getSingleton();

  // Creating an Ogre::RenderWindow initialises the render system, which is
  // necessary to create shaders (including reading scripts), so this has to be
  // done early.
  ctx.sdlInit = std::make_unique<sdl::Init>();
  const auto[windowWidth, windowHeight]{getWindowDimensions()};
  const auto windowFlags{getWindowFlags()};
  ctx.windows = oo::Window(ctx.ogreRoot.get(), windowWidth, windowHeight,
                           windowName, windowFlags);

  // Set the keyboard configuration
  oo::JobManager::runJob([&ctx = ctx, &gameSettings]() {
    ctx.keyMap = std::make_unique<oo::event::KeyMap>(gameSettings);
  });

  // Construct the Bullet configuration
  oo::JobManager::runJob([&ctx = ctx]() {
    ctx.bulletConf = std::make_unique<bullet::Configuration>();
  });

  // Set up the deferred rendering
  auto &compMgr{Ogre::CompositorManager::getSingleton()};
  compMgr.registerCustomCompositionPass("DeferredLight",
                                        ctx.deferredLightPass.get());

  // Construct the global terrain options
  oo::JobManager::runJob([this]() {
    this->ctx.terrainOptions = std::make_unique<Ogre::TerrainGlobalOptions>();
    this->setTerrainOptions();
  });

  // Start the sound engine
  ctx.soundMgr = std::make_unique<Ogre::SoundManager>();
  setSoundSettings();
  ctx.musicMgr = std::make_unique<oo::MusicManager>();

  // Start the developer console backend
  oo::JobManager::runJob([this]() {
    ctx.consoleEngine = std::make_unique<oo::ConsoleEngine>();
    registerConsoleFunctions();
  });

  // Star the scripting backend
  oo::JobManager::runJob([this]() {
    ctx.scriptEngine = std::make_unique<oo::ScriptEngine>();
    registerScriptFunctions();
  });

  // Add the resource managers
  oo::JobCounter managersAndFactoriesCounter{2};
  oo::JobManager::runJob([&ctx = ctx]() {
    ctx.meshMgr = std::make_unique<oo::MeshManager>();
    ctx.nifResourceMgr = std::make_unique<Ogre::NifResourceManager>();
    ctx.collisionObjectMgr = std::make_unique<Ogre::CollisionShapeManager>();
    ctx.textResourceMgr = std::make_unique<Ogre::TextResourceManager>();
    ctx.wavResourceMgr = std::make_unique<Ogre::WavResourceManager>();
  }, &managersAndFactoriesCounter);

  // Add the factories and codecs
  oo::JobManager::runJob([&ctx = ctx]() {
    ctx.rigidBodyFactory = std::make_unique<Ogre::RigidBodyFactory>();
    ctx.ogreRoot->addMovableObjectFactory(ctx.rigidBodyFactory.get());

    ctx.entityFactory = std::make_unique<oo::EntityFactory>();
    ctx.ogreRoot->addMovableObjectFactory(ctx.entityFactory.get());

    ctx.lightFactory = std::make_unique<oo::DeferredLightFactory>();
    ctx.ogreRoot->addMovableObjectFactory(ctx.lightFactory.get());

    ctx.scnMgrFactory = std::make_unique<oo::DeferredSceneManagerFactory>();
    ctx.ogreRoot->addSceneManagerFactory(ctx.scnMgrFactory.get());

    boost::this_fiber::yield();

    ctx.texImageCodec = std::make_unique<Ogre::TexImageCodec>();
    Ogre::Codec::registerCodec(ctx.texImageCodec.get());
  }, &managersAndFactoriesCounter);

  // Create the resolvers
  oo::JobCounter resolversCounter{1};
  oo::JobManager::runJob([&ctx = ctx]() {
    ctx.baseResolvers = std::make_unique<oo::BaseResolvers>(
        std::make_tuple(oo::add_resolver_t<record::RACE>{},
                        oo::add_resolver_t<record::LTEX>{},
                        oo::add_resolver_t<record::ACTI>{},
                        oo::add_resolver_t<record::CONT>{},
                        oo::add_resolver_t<record::DOOR>{},
                        oo::add_resolver_t<record::LIGH>{},
                        oo::add_resolver_t<record::MISC>{},
                        oo::add_resolver_t<record::STAT>{},
                        oo::add_resolver_t<record::GRAS>{},
                        oo::add_resolver_t<record::TREE>{},
                        oo::add_resolver_t<record::FLOR>{},
                        oo::add_resolver_t<record::FURN>{},
                        oo::add_resolver_t<record::NPC_>{},
                        oo::add_resolver_t<record::WTHR>{},
                        oo::add_resolver_t<record::CLMT>{},
                        oo::add_resolver_t<record::CELL>{*ctx.bulletConf},
                        oo::add_resolver_t<record::WRLD>{},
                        oo::add_resolver_t<record::LAND>{},
                        oo::add_resolver_t<record::WATR>{}));

    ctx.refrResolvers = std::make_unique<oo::RefrResolvers>(
        std::make_tuple(oo::add_refr_resolver_t<record::REFR_ACTI>{},
                        oo::add_refr_resolver_t<record::REFR_CONT>{},
                        oo::add_refr_resolver_t<record::REFR_DOOR>{},
                        oo::add_refr_resolver_t<record::REFR_LIGH>{},
                        oo::add_refr_resolver_t<record::REFR_MISC>{},
                        oo::add_refr_resolver_t<record::REFR_STAT>{},
                        oo::add_refr_resolver_t<record::REFR_FLOR>{},
                        oo::add_refr_resolver_t<record::REFR_FURN>{},
                        oo::add_refr_resolver_t<record::REFR_NPC_>{}));
  }, &resolversCounter);

  //===-------------------------------------------------------------------===//
  // Add resource groups and locations.
  // ResourceGroupManager is not thread safe so we can't use jobs here :(
  //===-------------------------------------------------------------------===//

  // Add the main resource group
  auto &resGrpMgr = Ogre::ResourceGroupManager::getSingleton();
  resGrpMgr.createResourceGroup(oo::RESOURCE_GROUP);

  // Shaders are not stored in the data folder (mostly for vcs reasons)
  resGrpMgr.addResourceLocation("./shaders", "FileSystem", oo::SHADER_GROUP);

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

  // Need managers before adding resources
  oo::JobManager::waitOn(&managersAndFactoriesCounter);

  // The bsa files need to be explicitly loaded before being added as resource
  // locations in order to guarantee thread safety.
  for (const auto &bsa : bsaFilenames) {
    const auto sysPath{bsa.sysPath()};
    Ogre::ArchiveManager::getSingleton().load(sysPath.string(), "BSA", true);
    ctx.getLogger()->info("Loaded archive {}", sysPath.string());
  }

  // Meshes need to be declared explicitly as they use a ManualResourceLoader.
  // We could just use Archive.SMasterMeshesArchiveFileName, but it is not
  // guaranteed to match any of Archive.sArchiveList, and also will not work for
  // any mod bsa files. While we're at it, we'll add the bsa files as resource
  // locations and declare every other recognised resource too.
  for (const auto &bsa : bsaFilenames) {
    this->declareBsaArchive(bsa);
    this->declareBsaResources(bsa);
  }

  declareFilesystemResources(dataPath);

  // All resources have been declared by now, so we can initialise the resource
  // groups. This won't initialise the default groups.
  resGrpMgr.initialiseResourceGroup(oo::RESOURCE_GROUP, false);
  resGrpMgr.initialiseResourceGroup(oo::SHADER_GROUP, true);

  declareMusic();

  ctx.imguiMgr = std::make_unique<Ogre::ImGuiManager>();

  // Need resolvers before reading esp files
  oo::JobManager::waitOn(&resolversCounter);

  // Load esp files
  const auto loadOrder{getLoadOrder(dataPath)};
  ctx.logger->info("Mod load order:");
  for (int i = 0; i < static_cast<int>(loadOrder.size()); ++i) {
    ctx.logger->info("0x{:0>2x} {}", i, loadOrder[i].view());
  }
  ctx.espCoordinator = std::make_unique<oo::EspCoordinator>(loadOrder.begin(),
                                                            loadOrder.end());
  // Read the main esm
  oo::JobCounter espCounter{1};
  oo::JobManager::runJob([&, &ctx = ctx]() {
    InitialRecordVisitor initialRecordVisitor(ctx.getBaseResolvers(),
                                              ctx.getRefrResolvers(),
                                              ctx.getPersistentReferenceLocator());
    for (int i = 0; i < static_cast<int>(loadOrder.size()); ++i) {
      oo::readEsp(*ctx.espCoordinator, i, initialRecordVisitor);
    }
  }, &espCounter);

  // Create the cell cache
  ctx.cellCache = std::make_unique<oo::CellCache>(
      gameSettings.get("General.uInterior Cell Buffer", 3u),
      std::max(gameSettings.get("General.uGridsToLoad", 3u)
                   * gameSettings.get("General.uGridsToLoad", 3u),
               gameSettings.get("General.uExterior Cell Buffer", 36u)),
      gameSettings.get("General.uWorld Buffer", 1u));

  createDummySceneManager();
  createDummyRenderQueue();

  modeStack.emplace_back(std::in_place_type<oo::MainMenuMode>, ctx);
  std::visit([this](auto &&state) {
    state.enter(ctx);
  }, modeStack.back());

  oo::JobManager::waitOn(&espCounter);
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

std::pair<int, int> Application::getWindowDimensions() const {
  const auto &gameSettings{oo::GameSettings::getSingleton()};
  auto logger{spdlog::get(oo::LOG)};

  const int windowWidth{gameSettings.iGet("Display.iSize W")};
  const int windowHeight{gameSettings.iGet("Display.iSize H")};
  if (windowHeight <= 0 || windowWidth <= 0) {
    logger->critical("Cannot create a window with width {} and height {}",
                     windowWidth, windowHeight);
    logger->critical(
        "Set 'Display.iSize W' and 'Display.iSize H' to sensible values");
    throw std::runtime_error("Cannot create window with negative size");
  }

  return {windowWidth, windowHeight};
}

sdl::WindowFlags Application::getWindowFlags() const {
  const auto &gameSettings{oo::GameSettings::getSingleton()};

  sdl::WindowFlags windowFlags{};
  if (gameSettings.bGet("Display.bFull Screen")) {
    windowFlags |= sdl::WindowFlags::Fullscreen;
  }

  return windowFlags;
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

void Application::setTerrainOptions() {
  const auto &gameSettings{oo::GameSettings::getSingleton()};
  const auto nearDiameter{gameSettings.get<unsigned int>(
      "General.uGridsToLoad", 3)};
  const auto nearRadius{(nearDiameter - 1u) / 2u};

  auto &options{ctx.terrainOptions};
  options->setMaxPixelError(8);
  options->setCompositeMapDistance(nearRadius * oo::unitsPerCell<Ogre::Real>
                                       * oo::metersPerUnit<Ogre::Real>);

  const auto lodTexSizePow = [&]() {
    auto pow{gameSettings.get<int>("LOD.iLODTextureSizePow2", 6)};
    return (pow <= 0 || pow > 16) ? 6 : pow;
  }();
  const auto lodTexSize{1u << static_cast<unsigned int>(lodTexSizePow)};

  options->setCompositeMapSize(static_cast<uint16_t>(lodTexSize));
  options->setDefaultMaterialGenerator(
      std::make_shared<oo::TerrainMaterialGenerator>());
  options->setLayerBlendMapSize(oo::verticesPerQuad<uint16_t>);
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
    resGrpMgr.declareResource(path.c_str(), "oo::Mesh",
                              resourceGroup, ctx.nifLoader.get());
    resGrpMgr.declareResource(path.c_str(), "CollisionShape",
                              resourceGroup, ctx.nifCollisionLoader.get());
    // TODO: Do all skeletons end with "skeleton.nif"?
    resGrpMgr.declareResource(path.c_str(), "Skeleton",
                              resourceGroup, ctx.skeletonLoader.get());
  } else if (ext == "kf"sv) {
    resGrpMgr.declareResource(path.c_str(), "Nif", resourceGroup);
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
  const auto sysPath{bsaFilename.sysPath().string()};
  resGrpMgr.addResourceLocation(sysPath, "BSA", oo::RESOURCE_GROUP);
}

void Application::declareBsaResources(const oo::Path &bsaFilename) {
  auto &archiveMgr{Ogre::ArchiveManager::getSingleton()};
  const auto sysPath{bsaFilename.sysPath().string()};
  const Ogre::Archive *archive{archiveMgr.load(sysPath, "BSA", true)};
  const Ogre::StringVectorPtr files{archive->list()};

  for (auto &filename : *files) {
    declareResource(oo::Path{std::move(filename)}, oo::RESOURCE_GROUP);
  }
}

void Application::declareFilesystemResources(const oo::Path &foldername) {
  auto &archiveMgr{Ogre::ArchiveManager::getSingleton()};
  const auto sysPath{foldername.sysPath().string()};
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

void Application::declareMusic() {
  auto &resGrpMgr{Ogre::ResourceGroupManager::getSingleton()};
  constexpr std::array<const char *, 4> musicPaths{
      "music/explore/*",
      "music/public/*",
      "music/dungeon/*",
      "music/battle/*"
  };
  for (uint32_t i = 0; i < musicPaths.size(); ++i) {
    const char *path{musicPaths[i]};
    auto fileList{resGrpMgr.findResourceNames(oo::RESOURCE_GROUP, path)};
    for (const auto &name : *fileList) {
      ctx.getMusicManager().addTrack(oo::MusicType{i}, name);
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
    return oo::Path{e.path().string()};
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
  rcf("ToggleOcclusionGeometry", &console::ToggleOcclusionGeometry);
  rcf("tog", &console::tog);
  rcf("ToggleFps", &console::ToggleFps);
  rcf("tfps", &console::tfps);
  rcf("ShowMainMenu", &console::ShowMainMenu);
  rcf("ShowClassMenu", &console::ShowClassMenu);
  rcf("ShowEnchantmentMenu", &console::ShowEnchantmentMenu);
  rcf("ShowMap", &console::ShowMap);
  rcf("ShowRaceMenu", &console::ShowRaceMenu);
  rcf("ShowSpellmaking", &console::ShowSpellmaking);
  rcf("print", &console::print);
  rcf("GetCurrentTime", &script::GetCurrentTime);
}

void Application::registerScriptFunctions() {
  auto rsf = [this](const std::string &name, auto F) {
    using F_t = std::remove_pointer_t<decltype(F)>;
    ctx.scriptEngine->registerFunction<F_t>(name);
  };

  rsf("GetCurrentTime", &script::GetCurrentTime);
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
  using Clock = std::chrono::high_resolution_clock;
  using fSecond = chrono::duration<float, chrono::seconds::period>;

  auto startTime{Clock::now()};

  ctx.getMusicManager().update(event.timeSinceLastFrame);

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

  // Keep yielding to other render fibers until we run out of time or exceed
  // some number of yields. This allows jobs that yield multiple times during
  // their execution to complete within a single frame, instead of having only
  // a subset between two adjacent yields be run each frame.
  float delta{0.0f};
  constexpr std::size_t MAX_YIELDS{20u};
  for (std::size_t i = 0; i < MAX_YIELDS && delta < event.timeSinceLastFrame;
       ++i) {
    boost::this_fiber::yield();
    delta += fSecond(Clock::now() - startTime).count();
  };

  return true;
}

bool Application::frameRenderingQueued(const Ogre::FrameEvent &/*event*/) {
  return true;
}

bool Application::frameEnded(const Ogre::FrameEvent &/*event*/) {
  return true;
}

} // namespace oo
