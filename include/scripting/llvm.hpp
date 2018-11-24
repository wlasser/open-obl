#ifndef OPENOBLIVION_SCRIPTING_LLVM_HPP
#define OPENOBLIVION_SCRIPTING_LLVM_HPP

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

  /// Create an alloca instruction in the entry block of the function.
  /// Use this for mutable local variables so that the mem2reg optimization pass
  /// can find them.
  /// \tparam Type The type of the variable to create an alloca instruction for.
  ///              Must be a RawShort, RawLong, RawFloat, or RawRef.
  template<class Type, class = std::enable_if_t<
      std::is_same_v<Type, grammar::RawShort>
          || std::is_same_v<Type, grammar::RawLong>
          || std::is_same_v<Type, grammar::RawFloat>
          || std::is_same_v<Type, grammar::RawRef>>>
  llvm::AllocaInst *createEntryBlockAlloca(llvm::Function *fun,
                                           const std::string &varName) {
    llvm::BasicBlock &entryBlock{fun->getEntryBlock()};
    llvm::IRBuilder irBuilder(&entryBlock, entryBlock.begin());

    llvm::Type *type{};
    // TODO: Treat references correctly
    if constexpr (std::is_same_v<Type, grammar::RawShort>) {
      type = llvm::Type::getInt16Ty(mCtx);
    } else if constexpr (std::is_same_v<Type, grammar::RawLong>) {
      type = llvm::Type::getInt32Ty(mCtx);
    } else if constexpr (std::is_same_v<Type, grammar::RawRef>) {
      type = llvm::Type::getInt32Ty(mCtx);
    } else {
      type = llvm::Type::getFloatTy(mCtx);
    }
    return irBuilder.CreateAlloca(type, nullptr, varName);
  }

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

  explicit LLVMVisitor(llvm::StringRef moduleName)
      : mIrBuilder(mCtx), mModule(moduleName, mCtx) {
    mPassManager.addPass(llvm::InstCombinePass{});
    mPassManager.addPass(llvm::NewGVNPass{});
    mPassManager.addPass(llvm::SimplifyCFGPass{});
    mPassManager.addPass(llvm::PromotePass{});
    llvm::PassBuilder passBuilder{};
    passBuilder.registerFunctionAnalyses(mAnalysisManager);
  }

  llvm::Value *visit(const AstNode &node);

  void print() {
    mModule.print(llvm::errs(), nullptr);
  }
};

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
LLVMVisitor::visitImpl<grammar::UnaryOperator>(const AstNode &node);

template<> llvm::Value *
LLVMVisitor::visitImpl<grammar::BinaryOperator>(const AstNode &node);

} // namespace oo

#endif // OPENOBLIVION_SCRIPTING_LLVM_HPP
