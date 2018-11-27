#ifndef OPENOBLIVION_SCRIPTING_SCRIPT_ENGINE_HPP
#define OPENOBLIVION_SCRIPTING_SCRIPT_ENGINE_HPP

#include "meta.hpp"
#include "scripting/ast.hpp"
#include "scripting/jit.hpp"
#include <llvm/IR/LLVMContext.h>
#include <memory>

extern "C" __attribute__((visibility("default"))) int Func(int x);

namespace oo {

class ScriptEngine {
 private:
  llvm::LLVMContext mCtx{};
  std::unique_ptr<oo::Jit> mJit{};
  llvm::StringMap<llvm::FunctionType *> mExternFuns{};

  auto jit(std::unique_ptr<llvm::Module> module);

  /// Create a prototype for a function returning Ret and taking Args as its
  /// arguments.
  template<class Ret, class ... Args> [[nodiscard]] llvm::FunctionType *
  makeProto(llvm::StringRef name, std::tuple<Args...> = {Args{}...});

  /// Convert a type from the AST into an LLVM type.
  // TODO: Treat references correctly
  template<class Type> [[nodiscard]] llvm::Type *typeToLLVM();

  template<class Fun> void addExternalFun(llvm::StringRef name);

  [[nodiscard]] std::unique_ptr<llvm::Module> compileAst(const AstNode &root);

 public:
  ScriptEngine();

  void compile(std::string_view script);

  template<class T> [[nodiscard]] T call(llvm::StringRef funName);
};

template<class Fun> void ScriptEngine::addExternalFun(llvm::StringRef name) {
  using T = function_traits<Fun>;
  mExternFuns[name] =
      makeProto<typename T::result_t>(name, typename T::args_t{});
}

template<class Ret, class ... Args> llvm::FunctionType *
ScriptEngine::makeProto(llvm::StringRef name, std::tuple<Args...>) {
  std::array<llvm::Type *, sizeof...(Args)> args{typeToLLVM<Args>() ...};
  return llvm::FunctionType::get(typeToLLVM<Ret>(), args, false);
}

template<class Type> llvm::Type *ScriptEngine::typeToLLVM() {
  if constexpr (std::is_same_v<Type, grammar::RawShort>
      || std::is_same_v<Type, short>) {
    return llvm::Type::getInt16Ty(mCtx);
  } else if constexpr (std::is_same_v<Type, grammar::RawLong>
      || std::is_same_v<Type, int>) {
    return llvm::Type::getInt32Ty(mCtx);
  } else if constexpr (std::is_same_v<Type, grammar::RawRef> ||
      std::is_same_v<Type, uint32_t>) {
    return llvm::Type::getInt32Ty(mCtx);
  } else if constexpr (std::is_same_v<Type, grammar::RawFloat> ||
      std::is_same_v<Type, float>) {
    return llvm::Type::getFloatTy(mCtx);
  } else {
    static_assert(false_v<Type>, "Type must be an AstType");
    llvm_unreachable("Type must be an AstType");
  }
}

template<class T> T ScriptEngine::call(llvm::StringRef funName) {
  auto entrySymbol{mJit->findSymbol(funName)};
  // TODO: Do something more reasonable if the function doesn't exist
  assert(entrySymbol && "Entry function not found");

  using entry_t = T (*)();
  auto entryAddrOrErr{entrySymbol.getAddress()};
  if (auto err{entryAddrOrErr.takeError()}) {
    llvm::errs() << err;
    throw std::runtime_error("Jit error");
  }
  auto entryAddr{reinterpret_cast<std::uintptr_t>(*entryAddrOrErr)};
  auto entry{reinterpret_cast<entry_t>(entryAddr)};
  const auto result{entry()};

  return result;
}

}

#endif // OPENOBLIVION_SCRIPTING_SCRIPT_ENGINE_HPP
