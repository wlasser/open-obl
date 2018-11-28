#include "scripting/llvm.hpp"
#include "scripting/pegtl.hpp"
#include "scripting/script_engine_base.hpp"
#include <llvm/Support/TargetSelect.h>

namespace oo {

ScriptEngineBase::ScriptEngineBase() {
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();

  mJit = std::make_unique<oo::Jit>();
}

oo::Jit *ScriptEngineBase::getJit() const noexcept {
  return mJit.get();
}

const llvm::StringMap<llvm::orc::VModuleKey> &
ScriptEngineBase::getModules() const noexcept {
  return mModules;
}

std::unique_ptr<llvm::Module>
ScriptEngineBase::makeModule(llvm::StringRef moduleName) {
  auto module{std::make_unique<llvm::Module>(moduleName, mCtx)};
  module->setDataLayout(mJit->getTargetMachine().createDataLayout());
  return module;
}

void ScriptEngineBase::jitModule(std::unique_ptr<llvm::Module> module) {
  llvm::StringRef moduleName{module->getName()};
  mModules[moduleName] = jit(std::move(module));
}

oo::LLVMVisitor ScriptEngineBase::makeVisitor(llvm::Module *module) {
  return LLVMVisitor(module, mCtx, mExternFuns);
}

llvm::orc::VModuleKey
ScriptEngineBase::jit(std::unique_ptr<llvm::Module> module) {
  return mJit->addModule(std::move(module));
}

} // namespace oo