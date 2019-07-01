#include "scripting/script_engine.hpp"
#include "scripting/logging.hpp"

namespace oo {

std::string ScriptEngine::getScriptname(const AstNode &node) {
  if (!node.is<grammar::RawScriptnameStatement>()) return "";
  if (node.children.size() != 2) return "";
  return node.children[1]->content();
}

std::unique_ptr<llvm::Module>
ScriptEngine::compileAst(const AstNode &root,
                         std::optional<uint32_t> calleeRef) {
  if (!root.is_root() || root.children.empty()) {
    // TODO: Cannot compile a partial AST, throw
    return nullptr;
  }

  const std::string moduleName{getScriptname(*root.children[0])};
  if (moduleName.empty()) {
    // TODO: Script has no name, throw
    return nullptr;
  }

  auto module{makeModule(moduleName)};
  addExternalFunsToModule(module.get());
  auto visitor{calleeRef ? makeVisitor(module.get(), *calleeRef)
                         : makeVisitor(module.get())};
  visitor.visit(root);

  return module;
}

std::optional<llvm::JITTargetAddress>
ScriptEngine::getFunctionAddr(const std::string &scriptName,
                              const std::string &funName) {
  // Find the module containing the script
  const auto keyIt{getModules().find(scriptName)};
  if (keyIt == getModules().end()) {
    oo::scriptingLogger()->warn("Script '{}' does not exist", scriptName);
    return std::nullopt;
  }

  // Find the function in the module
  auto entrySymbol{getJit()->findSymbolIn(funName, keyIt->second)};
  if (!entrySymbol) {
    oo::scriptingLogger()->warn("No function '{}' in script '{}'",
                                funName, scriptName);
    return std::nullopt;
  }

  // Get the function's address
  auto entryOrErr{entrySymbol.getAddress()};
  if (llvm::Error err = entryOrErr.takeError()) {
    llvm::handleAllErrors(std::move(err), [](const llvm::ErrorInfoBase &e) {
      oo::scriptingLogger()->warn("JIT error: {}", e.message());
    });
    return std::nullopt;
  }

  return *entryOrErr;
}

void ScriptEngine::compile(std::string_view script,
                           std::optional<uint32_t> calleeRef) {
  pegtl::memory_input in(script, "");

  const auto root{oo::parseScript(in)};
  if (!root) {
    // TODO: Script could not be parsed, throw
    return;
  }

  auto module{compileAst(*root, calleeRef)};
  if (!module) {
    // TODO: Failed to compile module, throw
    return;
  }

  jitModule(std::move(module));
}

} // namespace oo
