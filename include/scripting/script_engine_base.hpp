#ifndef OPENOBLIVION_SCRIPTING_SCRIPT_ENGINE_BASE_HPP
#define OPENOBLIVION_SCRIPTING_SCRIPT_ENGINE_BASE_HPP

#include "meta.hpp"
#include "scripting/ast.hpp"
#include "scripting/jit.hpp"
#include "scripting/llvm.hpp"
#include <llvm/IR/LLVMContext.h>
#include <memory>

namespace oo {

/// Common internal functionality of ScriptEngine and ConsoleEngine.
class ScriptEngineBase {
 private:
  std::unique_ptr<llvm::LLVMContext> mCtx{};
  std::unique_ptr<oo::Jit> mJit{};
  llvm::StringMap<llvm::FunctionType *> mExternFuns{};
  llvm::StringMap<llvm::orc::VModuleKey> mModules{};

 private:
  /// JIT and store the given module, returning a handle to it.
  llvm::orc::VModuleKey jit(std::unique_ptr<llvm::Module> module);

  /// Convert a type from the AST into an LLVM type.
  // TODO: Treat references correctly
  template<class Type> [[nodiscard]] llvm::Type *typeToLLVM();

  /// Create a prototype for a function returning Ret and taking Args as its
  /// arguments.
  template<class Ret, class ... Args> [[nodiscard]] llvm::FunctionType *
  makeProto(llvm::StringRef name, std::tuple<Args...> = {Args{}...});

 protected:
  [[nodiscard]] llvm::LLVMContext &getContext() noexcept;

  /// Declare all the registered external functions in the given module.
  /// This function can be run different modules, previously registered
  /// functions are remembered.
  void addExternalFunsToModule(llvm::Module *module);

  /// Register an internal (host process) function for use in all JIT'd modules.
  /// For example, if we have a function declaration
  /// `extern "C" int Func(float x)` then we can register it by calling
  /// `addExternalFun<decltype(Func)>("Func")`.
  template<class Fun> void addExternalFun(llvm::StringRef name);

  /// Get a pointer to the JIT compiler.
  [[nodiscard]] oo::Jit *getJit() const noexcept;

  /// Get a reference to the module name-handle map.
  /// \todo We already have jitModule to modify the module map, it would be
  ///       nice to expose a few of the const methods of the map through similar
  ///       functions instead of just exposing the entire map directly.
  [[nodiscard]] const llvm::StringMap<llvm::orc::VModuleKey> &
  getModules() const noexcept;

  /// Create a new empty module with the given name.
  [[nodiscard]] std::unique_ptr<llvm::Module>
  makeModule(llvm::StringRef moduleName);

  /// Take ownership of and JIT the given module.
  llvm::orc::VModuleKey jitModule(std::unique_ptr<llvm::Module> module);

  /// Create a new LLVMVisitor for the given module.
  [[nodiscard]] oo::LLVMVisitor makeVisitor(llvm::Module *module);

  /// \overload makeVisitor(llvm::Module *)
  [[nodiscard]] oo::LLVMVisitor makeVisitor(llvm::Module *module,
                                            llvm::IRBuilder<> builder);

  /// Create a new LLVMVisitor for the given module, called in the context of
  /// a reference `calleeRef`. If a function call is encountered but there is
  /// no known function with the same prototype as the call, then a `uint32_t`
  /// parameter will be prepended to the list of arguments and the lookup tried
  /// again. If a function is found, then the new function will be called in
  /// place of the original with the first argument set to `calleeRef` and all
  /// subsequent arguments set to the arguments of the original call, in order.
  [[nodiscard]] oo::LLVMVisitor makeVisitor(llvm::Module *module,
                                            uint32_t calleeRef);

  /// \overload makeVisitor(llvm::Module *, uint32_t)
  [[nodiscard]] oo::LLVMVisitor makeVisitor(llvm::Module *module,
                                            llvm::IRBuilder<> builder,
                                            uint32_t calleeRef);

  ScriptEngineBase();

 public:
  ~ScriptEngineBase() = default;
  ScriptEngineBase(const ScriptEngineBase &) = delete;
  ScriptEngineBase &operator=(const ScriptEngineBase &) = delete;

  ScriptEngineBase(ScriptEngineBase &&other) noexcept {
    using std::swap;
    swap(mCtx, other.mCtx);
    swap(mJit, other.mJit);
    swap(mExternFuns, other.mExternFuns);
    swap(mModules, other.mModules);
  }

  ScriptEngineBase &operator=(ScriptEngineBase &&other) noexcept {
    if (this != &other) {
      using std::swap;
      swap(mCtx, other.mCtx);
      swap(mJit, other.mJit);
      swap(mExternFuns, other.mExternFuns);
      swap(mModules, other.mModules);
    }
    return *this;
  }
};

template<class Fun>
void ScriptEngineBase::addExternalFun(llvm::StringRef name) {
  using T = function_traits<Fun>;
  mExternFuns[name] = makeProto<typename T::result_t>(name,
                                                      typename T::args_t{});
}

template<class Ret, class ... Args> llvm::FunctionType *
ScriptEngineBase::makeProto(llvm::StringRef /*name*/, std::tuple<Args...>) {
  std::array<llvm::Type *, sizeof...(Args)> args{typeToLLVM<Args>() ...};
  return llvm::FunctionType::get(typeToLLVM<Ret>(), args, false);
}

template<class Type> llvm::Type *ScriptEngineBase::typeToLLVM() {
  if constexpr (
      std::is_same_v<Type, grammar::RawShort> || std::is_same_v<Type, short>) {
    return llvm::Type::getInt16Ty(*mCtx);
  } else if constexpr (
      std::is_same_v<Type, grammar::RawLong> || std::is_same_v<Type, int>) {
    return llvm::Type::getInt32Ty(*mCtx);
  } else if constexpr (
      std::is_same_v<Type, grammar::RawRef> || std::is_same_v<Type, uint32_t>) {
    return llvm::Type::getInt32Ty(*mCtx);
  } else if constexpr (
      std::is_same_v<Type, grammar::RawFloat> || std::is_same_v<Type, float>) {
    return llvm::Type::getFloatTy(*mCtx);
  } else {
    static_assert(false_v<Type>, "Type must be an AstType");
    llvm_unreachable("Type must be an AstType");
  }
}

}

#endif // OPENOBLIVION_SCRIPTING_SCRIPT_ENGINE_BASE_HPP
