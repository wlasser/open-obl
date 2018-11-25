#ifndef OPENOBLIVION_SCRIPTING_LLVM_HPP
#define OPENOBLIVION_SCRIPTING_LLVM_HPP

#include "meta.hpp"
#include "scripting/ast.hpp"
#include "scripting/grammar.hpp"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar/NewGVN.h>
#include <llvm/Transforms/Scalar/SimplifyCFG.h>
#include <llvm/Transforms/Utils/Mem2Reg.h>
#include <optional>
#include <type_traits>

namespace oo {

class LLVMVisitor {
 private:
  llvm::LLVMContext mCtx{};
  llvm::IRBuilder<> mIrBuilder;
  llvm::Module mModule;
  llvm::FunctionPassManager mPassManager{};
  llvm::FunctionAnalysisManager mAnalysisManager{};
  llvm::StringMap<llvm::AllocaInst *> mNamedValues{};
  llvm::StringMap<llvm::GlobalVariable *> mGlobals{};
  llvm::StringMap<llvm::Function *> mFunctions{
      {"Func", makeProto<grammar::RawLong, grammar::RawLong>("Func")}
  };

  template<class NodeType, class = std::enable_if_t<AstSelector<NodeType>::value>>
  llvm::Value *visitImpl(const AstNode &node) {
    return nullptr;
  }

  /// Helper used in visit to write the NodeType only once per type, not twice.
  //C++20: Make this a lambda template in visit
  template<class NodeType>
  auto visitHelper(const AstNode &node) -> decltype(visitImpl<NodeType>(node)) {
    return node.is<NodeType>() ? visitImpl<NodeType>(node) : nullptr;
  }

  template<class Type>
  static constexpr inline bool
      isAstType = std::is_same_v<Type, grammar::RawShort>
      || std::is_same_v<Type, grammar::RawLong>
      || std::is_same_v<Type, grammar::RawRef>
      || std::is_same_v<Type, grammar::RawFloat>;

  /// Convert a type from the AST into an LLVM type.
  // TODO: Treat references correctly
  template<class Type> llvm::Type *typeToLLVM();

  /// Create an alloca instruction in the entry block of the function.
  /// Use this for mutable local variables so that the mem2reg optimization pass
  /// can find them.
  /// \tparam Type The type of the variable to create an alloca instruction for.
  ///              Must be a RawShort, RawLong, RawFloat, or RawRef.
  template<class Type, class = std::enable_if_t<isAstType<Type>>>
  llvm::AllocaInst *
  createEntryBlockAlloca(llvm::Function *fun, llvm::StringRef name);

  /// Create a prototype for a function returning Ret and taking Args as its
  /// arguments.
  template<class Ret, class ... Args>
  llvm::Function *makeProto(llvm::StringRef name);

  /// Promote/convert lhs and rhs to a common type.
  /// Emits instructions to convert lhs and rhs to a common type, if necessary,
  /// then returns the converted values. If a conversion is not necessary for an
  /// argument, then the argument is returned unmodified. The types conversions
  /// are performed as in C++:
  /// - If either operand is a `float`, the other operand is converted to a
  ///   `float`.
  /// - If one operand is a `short` and the other a `long`, the `short` operand
  ///   is converted to a `long`.
  [[nodiscard]] std::pair<llvm::Value *, llvm::Value *>
  promoteArithmeticOperands(llvm::Value *lhs, llvm::Value *rhs);

  /// Convert the argument to an i1.
  /// Emits instructions to convert lhs to an i1 if is not already an i1 by
  /// comparing against zero. In particular, the returned value is `0` if lhs is
  /// zero, and `1` otherwise.
  [[nodiscard]] llvm::Value *convertToBool(llvm::Value *lhs);

 public:
  explicit LLVMVisitor(llvm::StringRef moduleName);

  llvm::Value *visit(const AstNode &node);

  void print() {
    mModule.print(llvm::errs(), nullptr);
  }
};

template<class Type>
llvm::Type *LLVMVisitor::typeToLLVM() {
  if constexpr (std::is_same_v<Type, grammar::RawShort>) {
    return llvm::Type::getInt16Ty(mCtx);
  } else if constexpr (std::is_same_v<Type, grammar::RawLong>) {
    return llvm::Type::getInt32Ty(mCtx);
  } else if constexpr (std::is_same_v<Type, grammar::RawRef>) {
    return llvm::Type::getInt32Ty(mCtx);
  } else if constexpr (std::is_same_v<Type, grammar::RawFloat>) {
    return llvm::Type::getFloatTy(mCtx);
  } else {
    static_assert(false_v<Type>, "Type must be an AstType");
    return nullptr; // Unreachable
  }
}

template<class Type, class> llvm::AllocaInst *
LLVMVisitor::createEntryBlockAlloca(llvm::Function *fun, llvm::StringRef name) {
  llvm::BasicBlock &entryBlock{fun->getEntryBlock()};
  llvm::IRBuilder irBuilder(&entryBlock, entryBlock.begin());

  return irBuilder.CreateAlloca(typeToLLVM<Type>(), nullptr, name);
}

template<class Ret, class ... Args>
llvm::Function *LLVMVisitor::makeProto(llvm::StringRef name) {
  static_assert((isAstType<Ret> && ... && isAstType<Args>),
  "Ret and Args... must all be AstTypes");

  std::array<llvm::Type *, sizeof...(Args)> args{typeToLLVM<Args>() ...};
  auto *funType{llvm::FunctionType::get(typeToLLVM<Ret>(), args, false)};
  return llvm::Function::Create(funType, llvm::Function::ExternalLinkage,
                                name, &mModule);
}

template<> llvm::Value *
LLVMVisitor::visitImpl<grammar::RawScriptnameStatement>(const AstNode &node);

template<> llvm::Value *
LLVMVisitor::visitImpl<grammar::RawIdentifier>(const AstNode &node);

template<> llvm::Value *
LLVMVisitor::visitImpl<grammar::BlockStatement>(const AstNode &node);

template<> llvm::Value *
LLVMVisitor::visitImpl<grammar::IntegerLiteral>(const AstNode &node);

template<> llvm::Value *
LLVMVisitor::visitImpl<grammar::FloatLiteral>(const AstNode &node);

template<> llvm::Value *
LLVMVisitor::visitImpl<grammar::DeclarationStatement>(const AstNode &node);

template<> llvm::Value *
LLVMVisitor::visitImpl<grammar::SetStatement>(const AstNode &node);

template<> llvm::Value *
LLVMVisitor::visitImpl<grammar::ReturnStatement>(const AstNode &node);

template<> llvm::Value *
LLVMVisitor::visitImpl<grammar::RawCall>(const AstNode &node);

template<> llvm::Value *
LLVMVisitor::visitImpl<grammar::UnaryOperator>(const AstNode &node);

template<> llvm::Value *
LLVMVisitor::visitImpl<grammar::BinaryOperator>(const AstNode &node);

} // namespace oo

#endif // OPENOBLIVION_SCRIPTING_LLVM_HPP
