#ifndef OPENOBL_CONSOLE_ENGINE_HPP
#define OPENOBL_CONSOLE_ENGINE_HPP

#include "scripting/script_engine_base.hpp"
#include <nostdx/propagate_const.hpp>

namespace oo {

/// Compiles script statements entered into the developer console and runs them.
class ConsoleEngine : public ScriptEngineBase {
 private:
  class Impl;
  nostdx::propagate_const<std::unique_ptr<Impl>> mImpl;

 public:
  ConsoleEngine();
  ~ConsoleEngine();
  ConsoleEngine(const ConsoleEngine &) = delete;
  ConsoleEngine &operator=(const ConsoleEngine &) = delete;
  ConsoleEngine(ConsoleEngine &&) noexcept;
  ConsoleEngine &operator=(ConsoleEngine &&) noexcept;

  void execute(std::string_view statement);

  template<class Fun> void registerFunction(const std::string &funName);
};

template<class Fun>
void ConsoleEngine::registerFunction(const std::string &funName) {
  addExternalFun<Fun>(funName);
}

} // namespace oo

#endif // OPENOBL_CONSOLE_ENGINE_HPP
