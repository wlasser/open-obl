#ifndef OPENOBLIVION_SCRIPT_ENGINE_HPP
#define OPENOBLIVION_SCRIPT_ENGINE_HPP

#include "scripting/script_engine_base.hpp"

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
  template<class T>
  [[nodiscard]] T call(const std::string &scriptName,
                       const std::string &funName);
};

template<class T> T
ScriptEngine::call(const std::string &scriptName, const std::string &funName) {
  const auto keyIt{getModules().find(scriptName)};
  if (keyIt == getModules().end()) {
    // TODO: Do something more reasonable if the module doesn't exist
    assert(false && "No such script");
  }
  auto entrySymbol{getJit()->findSymbolIn(funName, keyIt->second)};
  // TODO: Do something more reasonable if the function doesn't exist
  assert(entrySymbol && "No such function");

  using entry_t = T (*)();
  auto entryAddrOrErr{entrySymbol.getAddress()};
  if (auto err{entryAddrOrErr.takeError()}) {
    llvm::errs() << err;
    throw std::runtime_error("Jit error");
  }
  auto entryAddr{reinterpret_cast<std::uintptr_t>(*entryAddrOrErr)};
  auto entry{reinterpret_cast<entry_t>(entryAddr)};
  const auto result{entry()};

  return result;
}

} // namespace oo

#endif //OPENOBLIVION_SCRIPT_ENGINE_HPP
