#include "ast.hpp"
#include "jit.hpp"
#include "llvm.hpp"
#include "scripting/logging.hpp"
#include "scripting/script_engine.hpp"

namespace oo {

class ScriptEngine::Impl {
 private:
  nostdx::propagate_const<ScriptEngine *> mParent;

 public:
  /// Get the scriptname from a RawScriptnameStatement.
  /// If `node` does not represent a RawScriptnameStatement, then an empty
  /// string is returned.template<class T> T
  [[nodiscard]] std::string getScriptname(const AstNode &node) const;

  /// Compile an entire AST into LLVM IR.
  /// \remark The returned module must still be JIT'd before it can be called.
  [[nodiscard]] std::unique_ptr<llvm::Module>
  compileAst(const AstNode &root, std::optional<uint32_t> calleeRef = {});

  explicit Impl(ScriptEngine *parent) noexcept : mParent(parent) {}
};

ScriptEngine::ScriptEngine()
    : ScriptEngineBase(), mImpl(std::make_unique<ScriptEngine::Impl>(this)) {}

ScriptEngine::~ScriptEngine() = default;
ScriptEngine::ScriptEngine(ScriptEngine &&) noexcept = default;
ScriptEngine &ScriptEngine::operator=(ScriptEngine &&) noexcept = default;

std::string ScriptEngine::Impl::getScriptname(const AstNode &node) const {
  if (!node.is<grammar::RawScriptnameStatement>()) return "";
  if (node.children.size() != 2) return "";
  return node.children[1]->content();
}

std::unique_ptr<llvm::Module>
ScriptEngine::Impl::compileAst(const AstNode &root,
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

  auto module{mParent->makeModule(moduleName)};
  mParent->addExternalFunsToModule(module.get());
  auto visitor{calleeRef ? mParent->makeVisitor(module.get(), *calleeRef)
                         : mParent->makeVisitor(module.get())};
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

  auto module{mImpl->compileAst(*root, calleeRef)};
  if (!module) {
    // TODO: Failed to compile module, throw
    return;
  }

  jitModule(std::move(module));
}

} // namespace oo
