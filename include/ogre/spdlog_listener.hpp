#ifndef OPENOBL_OGRE_SPDLOG_LISTENER_HPP
#define OPENOBL_OGRE_SPDLOG_LISTENER_HPP

#include <OgreLogManager.h>
#include <spdlog/spdlog.h>
#include <memory>

namespace Ogre {

// Adapter for intercepting Ogre's log messages and passing them to spdlog.
class SpdlogListener : public LogListener {
 public:

  explicit SpdlogListener(const String &loggerName) {
    mLogger = spdlog::get(loggerName);
  }

  void messageLogged(const String &message,
                     LogMessageLevel level,
                     bool /*maskDebug*/,
                     const String &/*logName*/,
                     bool &skipThisMessage) override {
    switch (level) {
      case LogMessageLevel::LML_TRIVIAL:mLogger->trace(message);
        break;
      case LogMessageLevel::LML_NORMAL:mLogger->info(message);
        break;
      case LogMessageLevel::LML_WARNING:mLogger->warn(message);
        break;
      case LogMessageLevel::LML_CRITICAL:mLogger->critical(message);
        break;
      default: return;
    }
    skipThisMessage = true;
  }

 private:
  std::shared_ptr<spdlog::logger> mLogger{};
};

} // namespace Ogre

#endif // OPENOBL_OGRE_SPDLOG_LISTENER_HPP
