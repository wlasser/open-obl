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

/// JIT compiler for the scripting system using LLVM.
class Jit {
 private:
  /// Type of the resource getter function passed to the object layer.
  using ResourceFunction = llvm::orc::RTDyldObjectLinkingLayer::ResourcesGetter;

  /// Type of the function used to create a llvm::orc::SymbolResolver.
  using LookupFunction = std::function<llvm::JITSymbol(
      const std::string &name)>;

  /// Type of the transformation functor passed to the optimization layer.
  using OptimizeFunction = std::function<std::unique_ptr<llvm::Module>(
      std::unique_ptr<llvm::Module>)>;

  /// ResourceGetter passed to the object layer, defined here to make the
  /// constructor shorter.
  ResourceFunction mResourceHelper = [this](llvm::orc::VModuleKey) {
    return llvm::orc::RTDyldObjectLinkingLayer::Resources{
        std::make_shared<llvm::SectionMemoryManager>(), mResolver};
  };

  /// Wrapper around lookup, to make the constructor shorter.
  LookupFunction mLookupHelper = [this](const std::string &name) {
    return lookup(name);
  };

  /// Wrapper around optimizeModule, to make the constructor shorter.
  OptimizeFunction mOptimizeHelper = [this](std::unique_ptr<llvm::Module> m) {
    return optimizeModule(std::move(m));
  };

  /// Wrapper around llvm::orc::createLegacyLookupResolver, to make the
  /// constructor shorter.
  template<class Lookup> static std::shared_ptr<llvm::orc::SymbolResolver>
  makeResolver(llvm::orc::ExecutionSession &session, Lookup &&legacyLookup) {
    return llvm::orc::createLegacyLookupResolver(
        session, legacyLookup, [](llvm::Error error) {
          llvm::cantFail(std::move(error), "lookupFlags failed");
        });
  }

  using ObjectLayer = llvm::orc::RTDyldObjectLinkingLayer;
  using CompileLayer = llvm::orc::IRCompileLayer<ObjectLayer,
                                                 llvm::orc::SimpleCompiler>;
  using OptimizeLayer = llvm::orc::IRTransformLayer<CompileLayer,
                                                    OptimizeFunction>;

  llvm::orc::ExecutionSession mSession{};
  std::shared_ptr<llvm::orc::SymbolResolver> mResolver;
  std::unique_ptr<llvm::TargetMachine> mTarget;
  llvm::DataLayout mDataLayout;
  ObjectLayer mObjectLayer;
  CompileLayer mCompileLayer;
  OptimizeLayer mOptimizeLayer;

  /// Take the given module, run a bunch of optimization passes on it, and
  /// return the now-optimized module.
  std::unique_ptr<llvm::Module>
  optimizeModule(std::unique_ptr<llvm::Module> module);

  /// Symbol lookup function for the llvm::orc::SymbolResolver.
  /// Looks for the symbol in the JIT'd modules first, then in the current
  /// process if one wasn't found. This makes it possible for scripts to
  /// override built-in functions for every other script; probably not a great
  /// idea, we'll see.
  llvm::JITSymbol lookup(const std::string &name);

 public:
  Jit();

  /// Return the target (host, in this case) machine.
  [[nodiscard]] llvm::TargetMachine &getTargetMachine() const noexcept;

  /// Take ownership of the given module and immediately compile it, returning a
  /// handle to the JIT'd module.
  llvm::orc::VModuleKey addModule(std::unique_ptr<llvm::Module> module);

  /// Get a (possible null) handle to the named symbol, across all JIT'd
  /// modules. If multiple JIT'd modules contain a symbol with the given name,
  /// it is undefined which one is returned.
  [[nodiscard]] llvm::JITSymbol findSymbol(llvm::StringRef name) noexcept;

  /// Get a (possibly null) handle to the named symbol in the specified module.
  [[nodiscard]] llvm::JITSymbol
  findSymbolIn(llvm::StringRef name, llvm::orc::VModuleKey key) noexcept;

  /// Unload the specified module, freeing memory.
  void removeModule(llvm::orc::VModuleKey key) noexcept;
};

} // namespace oo

#endif // OPENOBLIVION_SCRIPTING_JIT_HPP
