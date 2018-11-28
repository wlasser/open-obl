#ifndef OPENOBLIVION_SCRIPT_ENGINE_HPP
#define OPENOBLIVION_SCRIPT_ENGINE_HPP

#include "scripting/logging.hpp"
#include "scripting/script_engine_base.hpp"
#include <optional>

extern "C" __attribute__((visibility("default"))) int Func(int x);

namespace oo {

/// Compilers user scripts and makes them available for running at game time.
class ScriptEngine : public ScriptEngineBase {
 private:
  /// Get the scriptname from a RawScriptnameStatement.
  /// If `node` does not represent a RawScriptnameStatement, then an empty
  /// string is returned.template<class T> T
  [[nodiscard]] std::string getScriptname(const AstNode &node);

  /// Compile an entire AST into LLVM IR.
  /// \remark The returned module must still be JIT'd before it can be called.
  [[nodiscard]] std::unique_ptr<llvm::Module> compileAst(const AstNode &root);

 public:
  ScriptEngine();

  /// Compile a script into native object code, making it available for calling.
  void compile(std::string_view script);

  /// Call the given function from the given script.
  /// The given script must been `compile`d previously, and a function with the
  /// given name and specified return type must exist in the script.
  /// \tparam T The return type of the function.
  // TODO: Make this take std::string_view with a conversion to llvm::StringRef
  template<class T> [[nodiscard]] std::optional<T>
  call(const std::string &scriptName, const std::string &funName);
};

template<class T> std::optional<T>
ScriptEngine::call(const std::string &scriptName, const std::string &funName) {
  // Find the module containing the script
  const auto keyIt{getModules().find(scriptName)};
  if (keyIt == getModules().end()) {
    scriptingLogger()->warn("Script '{}' does not exist", scriptName);
    return std::nullopt;
  }

  // Find the function in the module
  auto entrySymbol{getJit()->findSymbolIn(funName, keyIt->second)};
  if (!entrySymbol) {
    scriptingLogger()->warn("No function '{}' in script '{}'",
                            funName, scriptName);
    return std::nullopt;
  }

  // Get the function's address
  auto entryOrErr{entrySymbol.getAddress()};
  if (llvm::Error err = entryOrErr.takeError()) {
    llvm::handleAllErrors(std::move(err), [](const llvm::ErrorInfoBase &e) {
      scriptingLogger()->warn("JIT error: {}", e.message());
    });
    return std::nullopt;
  }

  // Convert the address to a function pointer and call it.
  using entry_t = T (*)();
  auto entryAddr{reinterpret_cast<std::uintptr_t>(*entryOrErr)};
  auto entry{reinterpret_cast<entry_t>(entryAddr)};
  const auto result{entry()};

  return result;
}

} // namespace oo

#endif //OPENOBLIVION_SCRIPT_ENGINE_HPP
