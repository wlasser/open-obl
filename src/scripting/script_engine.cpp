#include "scripting/script_engine.hpp"

int Func(int x) {
  return 9 * x;
}

namespace oo {

ScriptEngine::ScriptEngine() : ScriptEngineBase() {
  addExternalFun<decltype(Func)>("Func");
}

std::string ScriptEngine::getScriptname(const AstNode &node) {
  if (!node.is<grammar::RawScriptnameStatement>()) return "";
  if (node.children.size() != 2) return "";
  return node.children[1]->content();
}

std::unique_ptr<llvm::Module> ScriptEngine::compileAst(const AstNode &root) {
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
  auto visitor{makeVisitor(module.get())};
  visitor.visit(root);

  return module;
}

void ScriptEngine::compile(std::string_view script) {
  pegtl::memory_input in(script, "");

  const auto root{oo::parseScript(in)};
  if (!root) {
    // TODO: Script could not be parsed, throw
    return;
  }

  auto module{compileAst(*root)};
  if (!module) {
    // TODO: Failed to compile module, throw
    return;
  }

  jitModule(std::move(module));
}

} // namespace oo
