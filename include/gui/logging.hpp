#ifndef OPENOBLIVION_GUI_LOGGING_HPP
#define OPENOBLIVION_GUI_LOGGING_HPP

#include <spdlog/spdlog.h>
#include <spdlog/sinks/null_sink.h>
#include <memory>
#include <optional>
#include <string>

namespace gui {

inline std::shared_ptr<spdlog::logger>
guiLogger(std::optional<std::string> loggerName = std::nullopt) {
  static std::shared_ptr<spdlog::logger> logger = [&]() {
    if (loggerName) return spdlog::get(*loggerName);
    auto sink{std::make_shared<spdlog::sinks::null_sink_st>()};
    return std::make_shared<spdlog::logger>("guiLogger", sink);
  }();
  return logger;
}

} // namespace gui

#endif // OPENOBLIVION_GUI_LOGGING_HPP
