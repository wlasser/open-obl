#ifndef OPENOBLIVION_CONSOLE_ENGINE_HPP
#define OPENOBLIVION_CONSOLE_ENGINE_HPP

#include "scripting/logging.hpp"
#include "scripting/script_engine_base.hpp"
#include <optional>

namespace oo {

/// Compiles script statements entered into the developer console and runs them.
class ConsoleEngine : public ScriptEngineBase {
 private:
  [[nodiscard]] std::unique_ptr<llvm::Module>
  compileStatement(const AstNode &node);

 public:
  ConsoleEngine() : ScriptEngineBase() {}

  void execute(std::string_view statement);

  template<class Fun> void registerFunction(const std::string &funName);
};

template<class Fun>
void ConsoleEngine::registerFunction(const std::string &funName) {
  addExternalFun<Fun>(funName);
}

} // namespace oo

#endif // OPENOBLIVION_CONSOLE_ENGINE_HPP
