#include "application_base/application_base.hpp"
#include "ogre/spdlog_listener.hpp"
#include "util/settings.hpp"

#include <OgreRoot.h>

#include <spdlog/sinks/basic_file_sink.h>
#if defined(_WIN32) || defined(_WIN64)
#include <spdlog/sinks/stdout_sinks.h>
#else
#include <spdlog/sinks/stdout_color_sinks.h>
#endif

namespace oo {

std::unique_ptr<Ogre::LogListener>
createLoggers(const std::string &filename, Ogre::LogManager &logMgr) {
#if defined(_WIN32) || defined(_WIN64)
  using ColorSink = spdlog::sinks::stdout_sink_mt;
#else
  using ColorSink = spdlog::sinks::stdout_color_sink_mt;
#endif
  using FileSink = spdlog::sinks::basic_file_sink_mt;

  // The console gets info and above, in particular not debug
  auto consoleSink{std::make_shared<ColorSink>()};
  consoleSink->set_level(spdlog::level::info);

  // The log file gets everything
  auto fileSink{std::make_shared<FileSink>(filename, true)};
  fileSink->set_level(spdlog::level::trace);

  // Every log will write to both the console and the log file
  const std::initializer_list<spdlog::sink_ptr> sinks{consoleSink, fileSink};

  // Construct the default Ogre logger and register its spdlog listener
  auto ogreLogger{std::make_shared<spdlog::logger>(oo::OGRE_LOG, sinks)};
  spdlog::register_logger(ogreLogger);
  auto *defaultLog{logMgr.createLog("Default", true, true, true)};
  auto logListener = std::make_unique<Ogre::SpdlogListener>(oo::OGRE_LOG);
  defaultLog->addListener(logListener.get());

  // Construct our own logger
  auto logger{std::make_shared<spdlog::logger>(oo::LOG, sinks)};
  spdlog::register_logger(logger);

  // Set the starting logger levels. These will be modified later according to
  // user settings, if specified.
  logger->set_level(spdlog::level::warn);
  ogreLogger->set_level(spdlog::level::info);

  return logListener;
}

Ogre::RenderSystem *
setRenderSystem(Ogre::Root *root, const std::string &systemName) {
  if (auto *renderSystem = root->getRenderSystemByName(systemName)) {
    root->setRenderSystem(renderSystem);
    return renderSystem;
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

std::unique_ptr<Ogre::GL3PlusPlugin> startGl3Plus(Ogre::Root *root) {
  auto plugin{std::make_unique<Ogre::GL3PlusPlugin>()};
  root->installPlugin(plugin.get());
  // Sadly the plugin name is different to the render system name, so we can't
  // ask plugin for which render subsystem to look for.
  // TODO: Add `getRenderSystemName()` function to the OGRE render system
  //       plugins.
  oo::setRenderSystem(root, "OpenGL 3+ Rendering Subsystem");

  return plugin;
}

Window::Window() : mSdlWin(nullptr, nullptr), mOgreWin(nullptr) {}

Window::Window(Ogre::Root *root, int width, int height, const std::string &name,
               sdl::WindowFlags flags)
    : mSdlWin(sdl::makeWindow(name, width, height, flags)),
      mOgreWin(nullptr) {
  const auto winInfo{sdl::getSysWMInfo(mSdlWin.get())};
  const auto parent{sdl::getWindowParent(winInfo)};

  // Construct a render window with the SDL window as a parent; SDL handles the
  // window itself, OGRE manages the OpenGL context.
  const Ogre::NameValuePairList params{
      {"parentWindowHandle", parent}
  };
  mOgreWin = Ogre::makeRenderWindow(root, name,
                                    static_cast<unsigned>(width),
                                    static_cast<unsigned>(height),
                                    &params);
}

} // namespace oo
