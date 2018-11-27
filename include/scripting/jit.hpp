#ifndef OPENOBLIVION_SCRIPTING_JIT_HPP
#define OPENOBLIVION_SCRIPTING_JIT_HPP

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/Core.h>
#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/IRTransformLayer.h>
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
  using OptimizeFunction = std::function<std::unique_ptr<llvm::Module>(
      std::unique_ptr<llvm::Module>)>;

  std::function<llvm::JITSymbol(const std::string &name)> mLegacyLookup =
      [this](const std::string &name) -> llvm::JITSymbol {
        // Look for symbol in JIT'd module first
        if (auto symbol{mCompileLayer.findSymbol(name, false)}) {
          return symbol;
        } else if (auto error{symbol.takeError()}) {
          return std::move(error);
        }

        // Look in the current process if the symbol wasn't found
        if (auto
            addr{llvm::RTDyldMemoryManager::getSymbolAddressInProcess(name)}) {
          return llvm::JITSymbol(addr, llvm::JITSymbolFlags::Exported);
        }

        return nullptr;
      };

  OptimizeFunction mOptimizeHelper = [this](std::unique_ptr<llvm::Module> m) {
    return optimizeModule(std::move(m));
  };

  /// Helper function for use in the constructor.
  template<class Lookup>
  static std::shared_ptr<llvm::orc::SymbolResolver>
  makeResolver(llvm::orc::ExecutionSession &session, Lookup &&legacyLookup) {
    return llvm::orc::createLegacyLookupResolver(
        session, legacyLookup, [](llvm::Error error) {
          llvm::cantFail(std::move(error), "lookupFlags failed");
        });
  };

  llvm::orc::ExecutionSession mSession{};
  std::shared_ptr<llvm::orc::SymbolResolver> mResolver;
  std::unique_ptr<llvm::TargetMachine> mTarget;
  llvm::DataLayout mDataLayout;
  llvm::orc::RTDyldObjectLinkingLayer mObjectLayer;
  llvm::orc::IRCompileLayer<decltype(mObjectLayer), llvm::orc::SimpleCompiler>
      mCompileLayer;
  llvm::orc::IRTransformLayer<decltype(mCompileLayer), OptimizeFunction>
      mOptimizeLayer;

  std::unique_ptr<llvm::Module>
  optimizeModule(std::unique_ptr<llvm::Module> module);

 public:
  Jit();

  [[nodiscard]] llvm::TargetMachine &getTargetMachine() const noexcept;

  llvm::orc::VModuleKey addModule(std::unique_ptr<llvm::Module> module);

  [[nodiscard]] llvm::JITSymbol findSymbol(llvm::StringRef name) noexcept;

  [[nodiscard]] llvm::JITSymbol
  findSymbolIn(llvm::StringRef name, llvm::orc::VModuleKey key) noexcept;

  [[nodiscard]] llvm::JITTargetAddress
  getSymbolAddress(const std::string &name) noexcept;

  void removeModule(llvm::orc::VModuleKey key) noexcept;
};

} // namespace oo

#endif // OPENOBLIVION_SCRIPTING_JIT_HPP
