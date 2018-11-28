#ifndef OPENOBLIVION_SCRIPTING_LOGGING_HPP
#define OPENOBLIVION_SCRIPTING_LOGGING_HPP

#include <spdlog/spdlog.h>
#include <spdlog/sinks/null_sink.h>
#include <optional>
#include <string>

namespace oo {

/// Call this function with a logger name before instantiating a ScriptEngine or
/// ConsoleEngine to set the name of the spdlog logger for the library to use.
/// Not doing so, or calling it with an empty argument, will make the library
/// use a default null logger with no output.
std::shared_ptr<spdlog::logger>
inline scriptingLogger(std::optional<std::string> loggerName = std::nullopt) {
  static std::shared_ptr<spdlog::logger> logger = [&]() {
    if (loggerName) return spdlog::get(*loggerName);
    auto sink{std::make_shared<spdlog::sinks::null_sink_st>()};
    return std::make_shared<spdlog::logger>("scriptingLogger", sink);
  }();

  return logger;
}

}

#endif // OPENOBLIVION_SCRIPTING_LOGGING_HPP
