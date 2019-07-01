#ifndef OPENOBL_CONSOLE_ENGINE_HPP
#define OPENOBL_CONSOLE_ENGINE_HPP

#include "scripting/script_engine_base.hpp"

namespace oo {

/// Compiles script statements entered into the developer console and runs them.
class ConsoleEngine : public ScriptEngineBase {
 private:
  [[nodiscard]] std::unique_ptr<llvm::Module>
  compileStatement(const AstNode &node);

 public:
  ConsoleEngine() : ScriptEngineBase() {}
  ~ConsoleEngine() = default;

  void execute(std::string_view statement);

  template<class Fun> void registerFunction(const std::string &funName);
};

template<class Fun>
void ConsoleEngine::registerFunction(const std::string &funName) {
  addExternalFun<Fun>(funName);
}

} // namespace oo

#endif // OPENOBL_CONSOLE_ENGINE_HPP
