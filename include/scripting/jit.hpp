#ifndef OPENOBLIVION_SCRIPTING_JIT_HPP
#define OPENOBLIVION_SCRIPTING_JIT_HPP

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/Core.h>
#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/Legacy.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>
#include <llvm/ExecutionEngine/RTDyldMemoryManager.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Mangler.h>
#include <llvm/Target/TargetMachine.h>
#include <memory>

namespace oo {

class Jit {
 private:
  std::function<llvm::JITSymbol(const std::string &name)>
      mLegacyLookup = [this](const std::string &name) -> llvm::JITSymbol {
    // Look for symbol in JIT'd module first
    if (auto symbol{mCompileLayer.findSymbol(name, false)}) {
      return symbol;
    } else if (auto error{symbol.takeError()}) {
      return std::move(error);
    }

    // Look in the current process if the symbol wasn't found
    if (auto addr{llvm::RTDyldMemoryManager::getSymbolAddressInProcess(name)}) {
      return llvm::JITSymbol(addr, llvm::JITSymbolFlags::Exported);
    }

    return nullptr;
  };

  llvm::orc::ExecutionSession mSession{};
  std::shared_ptr<llvm::orc::SymbolResolver> mResolver;
  std::unique_ptr<llvm::TargetMachine> mTarget;
  llvm::DataLayout mDataLayout;
  llvm::orc::RTDyldObjectLinkingLayer mObjectLayer;
  llvm::orc::IRCompileLayer<decltype(mObjectLayer), llvm::orc::SimpleCompiler>
      mCompileLayer;

 public:
  Jit() : mResolver{llvm::orc::createLegacyLookupResolver(
      mSession, mLegacyLookup, [](llvm::Error error) {
        llvm::cantFail(std::move(error), "lookupFlags failed");
      })},
          mTarget{llvm::EngineBuilder{}.selectTarget()},
          mDataLayout{mTarget->createDataLayout()},
          mObjectLayer(mSession, [this](llvm::orc::VModuleKey) {
            return llvm::orc::RTDyldObjectLinkingLayer::Resources{
                std::make_shared<llvm::SectionMemoryManager>(),
                mResolver};
          }),
          mCompileLayer(mObjectLayer, llvm::orc::SimpleCompiler(*mTarget)) {
    // Load the containing process as a library, making all its exported symbols
    // available for calling in JIT'd code.
    llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr);
  }

  llvm::TargetMachine &getTargetMachine() const noexcept {
    return *mTarget;
  }

  llvm::orc::VModuleKey addModule(std::unique_ptr<llvm::Module> module) {
    const auto key{mSession.allocateVModule()};
    llvm::cantFail(mCompileLayer.addModule(key, std::move(module)));
    return key;
  }

  llvm::JITSymbol findSymbol(const std::string &name) {
    std::string mangledName{};
    llvm::raw_string_ostream mangledNameStream(mangledName);
    llvm::Mangler::getNameWithPrefix(mangledNameStream, name, mDataLayout);
    return mCompileLayer.findSymbol(mangledNameStream.str(), true);
  }

  llvm::JITTargetAddress getSymbolAddress(const std::string &name) noexcept {
    return llvm::cantFail(findSymbol(name).getAddress());
  }

  void removeModule(llvm::orc::VModuleKey key) noexcept {
    llvm::cantFail(mCompileLayer.removeModule(key));
  }
};

} // namespace oo

#endif // OPENOBLIVION_SCRIPTING_JIT_HPP
