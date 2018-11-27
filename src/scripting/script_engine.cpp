#include "scripting/llvm.hpp"
#include "scripting/pegtl.hpp"
#include "scripting/script_engine.hpp"
#include <llvm/Support/TargetSelect.h>

int Func(int x) {
  return 9 * x;
}

namespace oo {

ScriptEngine::ScriptEngine() {
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();

  mJit = std::make_unique<oo::Jit>();
  addExternalFun<decltype(Func)>("Func");
}

llvm::orc::VModuleKey ScriptEngine::jit(std::unique_ptr<llvm::Module> module) {
  return mJit->addModule(std::move(module));
}

[[nodiscard]] std::unique_ptr<llvm::Module>
ScriptEngine::compileAst(const AstNode &root) {
  if (!root.is_root()) {
    // TODO: Cannot compile a partial AST, throw
    return nullptr;
  }

  // If the first child of the root is a RawScriptnameStatement, then name the
  // module according to that. Otherwise, this is an anonymous module so come up
  // with a name.
  std::string moduleName{"anonymous"};
  if (!root.children.empty()) {
    const auto &scnStatement{root.children[0]};
    if (scnStatement->is<grammar::RawScriptnameStatement>()
        && scnStatement->children.size() == 2) {
      moduleName = scnStatement->children[1]->content();
    }
  }

  LLVMVisitor visitor(moduleName, mCtx,
                      mJit->getTargetMachine().createDataLayout(),
                      mExternFuns);
  visitor.visit(root);

  return std::move(visitor.mModule);
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

  llvm::StringRef moduleName{module->getName()};

  mModules[moduleName] = jit(std::move(module));
}

} // namespace oo
