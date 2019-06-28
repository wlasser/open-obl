#ifndef OPENOBL_NIFLOADER_LOGGING_HPP
#define OPENOBL_NIFLOADER_LOGGING_HPP

#include <spdlog/spdlog.h>
#include <spdlog/sinks/null_sink.h>
#include <optional>
#include <string>
#include "windows_cleanup.hpp"

namespace oo {

/// Call this function before using the library to set the name of the spdlog
/// logger for the library to use. Not doing so, or calling it with an empty
/// argument, will make the library use a default null logger with no output.
std::shared_ptr<spdlog::logger>
inline nifloaderLogger(std::optional<std::string> loggerName = std::nullopt) {
  static std::shared_ptr<spdlog::logger> logger = [&]() {
    if (loggerName) return spdlog::get(*loggerName);
    auto sink{std::make_shared<spdlog::sinks::null_sink_st>()};
    return std::make_shared<spdlog::logger>("nifloaderLogger", sink);
  }();

  return logger;
}

} // namespace oo

#endif // OPENOBL_NIFLOADER_LOGGING_HPP
