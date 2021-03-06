#include "jit.hpp"
#include <llvm/Passes/PassBuilder.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar/NewGVN.h>
#include <llvm/Transforms/Scalar/SimplifyCFG.h>
#include <llvm/Transforms/Utils/Mem2Reg.h>

namespace oo {

llvm::JITSymbol Jit::lookup(const std::string &name) {
  if (auto symbol{mCompileLayer.findSymbol(name, false)}) {
    return symbol;
  } else if (auto error{symbol.takeError()}) {
    return std::move(error);
  }

  if (auto addr{llvm::RTDyldMemoryManager::getSymbolAddressInProcess(name)}) {
    return llvm::JITSymbol(addr, llvm::JITSymbolFlags::Exported);
  }

  return nullptr;
}

std::unique_ptr<llvm::Module>
Jit::optimizeModule(std::unique_ptr<llvm::Module> module) {
  llvm::FunctionPassManager passMgr{};
  llvm::FunctionAnalysisManager analysisMgr{};

  passMgr.addPass(llvm::InstCombinePass{});
  passMgr.addPass(llvm::NewGVNPass{});
  passMgr.addPass(llvm::SimplifyCFGPass{});
  passMgr.addPass(llvm::PromotePass{});
  llvm::PassBuilder passBuilder{};
  passBuilder.registerFunctionAnalyses(analysisMgr);

  for (auto &fun : *module) {
    if (!fun.getBasicBlockList().empty()) {
      passMgr.run(fun, analysisMgr);
    }
  }

  return module;
}

Jit::Jit() : mResolver{makeResolver(mSession, mLookupHelper)},
             mTarget{llvm::EngineBuilder{}.selectTarget()},
             mDataLayout{mTarget->createDataLayout()},
             mObjectLayer(mSession, mResourceHelper),
             mCompileLayer(mObjectLayer, llvm::orc::SimpleCompiler(*mTarget)),
             mOptimizeLayer(mCompileLayer, mOptimizeHelper) {
  // Load the containing process as a library, making all its exported symbols
  // available for calling in JIT'd code.
  llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr);
}

llvm::TargetMachine &Jit::getTargetMachine() const noexcept {
  return *mTarget;
}

llvm::orc::VModuleKey Jit::addModule(std::unique_ptr<llvm::Module> module) {
  const auto key{mSession.allocateVModule()};
  llvm::cantFail(mOptimizeLayer.addModule(key, std::move(module)));
  return key;
}

llvm::JITSymbol Jit::findSymbol(llvm::StringRef name) noexcept {
  std::string mangledName{};
  llvm::raw_string_ostream mangledNameStream(mangledName);
  llvm::Mangler::getNameWithPrefix(mangledNameStream, name, mDataLayout);
  return mOptimizeLayer.findSymbol(mangledNameStream.str(), true);
}

llvm::JITSymbol
Jit::findSymbolIn(llvm::StringRef name, llvm::orc::VModuleKey key) noexcept {
  std::string mangledName{};
  llvm::raw_string_ostream mangledNameStream(mangledName);
  llvm::Mangler::getNameWithPrefix(mangledNameStream, name, mDataLayout);
  return mOptimizeLayer.findSymbolIn(key, mangledNameStream.str(), true);
}

void Jit::removeModule(llvm::orc::VModuleKey key) noexcept {
  llvm::cantFail(mOptimizeLayer.removeModule(key));
}

} // namespace oo