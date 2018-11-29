#ifndef OPENOBLIVION_CONSOLE_ENGINE_HPP
#define OPENOBLIVION_CONSOLE_ENGINE_HPP

#include "scripting/logging.hpp"
#include "scripting/script_engine_base.hpp"
#include <optional>

extern "C" __attribute__((visibility("default"), used)) int ConsoleFunc(int x);

namespace oo {

/// Compiles script statements entered into the developer console and runs them.
class ConsoleEngine : public ScriptEngineBase {
 private:
  [[nodiscard]] std::unique_ptr<llvm::Module>
  compileStatement(const AstNode &node);

 public:
  ConsoleEngine();

  void execute(std::string_view statement);
};

} // namespace oo

#endif // OPENOBLIVION_CONSOLE_ENGINE_HPP
