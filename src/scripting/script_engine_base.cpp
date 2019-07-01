#include "jit.hpp"
#include "llvm.hpp"
#include "pegtl.hpp"
#include "scripting/script_engine_base.hpp"
#include <llvm/Support/TargetSelect.h>

namespace oo {

ScriptEngineBase::ScriptEngineBase()
    : mCtx(std::make_unique<llvm::LLVMContext>()) {
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();

  mJit = std::make_unique<oo::Jit>();
}

ScriptEngineBase::~ScriptEngineBase() = default;

ScriptEngineBase::ScriptEngineBase(ScriptEngineBase &&other) noexcept {
  using std::swap;
  swap(mCtx, other.mCtx);
  swap(mJit, other.mJit);
  swap(mExternFuns, other.mExternFuns);
  swap(mModules, other.mModules);
}

ScriptEngineBase &
ScriptEngineBase::operator=(ScriptEngineBase &&other) noexcept {
  if (this != &other) {
    using std::swap;
    swap(mCtx, other.mCtx);
    swap(mJit, other.mJit);
    swap(mExternFuns, other.mExternFuns);
    swap(mModules, other.mModules);
  }
  return *this;
}

llvm::LLVMContext &ScriptEngineBase::getContext() noexcept {
  return *mCtx;
}

void ScriptEngineBase::addExternalFunsToModule(llvm::Module *module) {
  const auto linkage{llvm::Function::ExternalLinkage};
  for (const auto &entry : mExternFuns) {
    std::string funName{entry.getKey()};
    llvm::FunctionType *funType{entry.second};
    llvm::Function::Create(funType, linkage, funName, module);
  }
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
  auto module{std::make_unique<llvm::Module>(moduleName, *mCtx)};
  module->setDataLayout(mJit->getTargetMachine().createDataLayout());
  return module;
}

llvm::orc::VModuleKey
ScriptEngineBase::jitModule(std::unique_ptr<llvm::Module> module) {
  std::string moduleName{module->getName()};
  return (mModules[moduleName] = jit(std::move(module)));
}

oo::LLVMVisitor ScriptEngineBase::makeVisitor(llvm::Module *module) {
  return LLVMVisitor(module, *mCtx);
}

oo::LLVMVisitor ScriptEngineBase::makeVisitor(llvm::Module *module,
                                              llvm::IRBuilder<> builder) {
  return LLVMVisitor(module, *mCtx, std::move(builder));
}

oo::LLVMVisitor ScriptEngineBase::makeVisitor(llvm::Module *module,
                                              uint32_t calleeRef) {
  return LLVMVisitor(module, *mCtx, {}, calleeRef);
}

oo::LLVMVisitor ScriptEngineBase::makeVisitor(llvm::Module *module,
                                              llvm::IRBuilder<> builder,
                                              uint32_t calleeRef) {
  return LLVMVisitor(module, *mCtx, std::move(builder), calleeRef);
}

llvm::orc::VModuleKey
ScriptEngineBase::jit(std::unique_ptr<llvm::Module> module) {
  return mJit->addModule(std::move(module));
}

} // namespace oo