#ifndef OPENOBLIVION_SCRIPTING_LLVM_HPP
#define OPENOBLIVION_SCRIPTING_LLVM_HPP

#include "scripting/grammar.hpp"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <optional>

namespace scripting {

class LLVMVisitor {
 private:
  llvm::LLVMContext mCtx{};
  llvm::IRBuilder<> mIrBuilder;
  llvm::Module mModule;
  llvm::StringMap<llvm::AllocaInst *> mNamedValues{};

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
      std::is_same_v<Type, RawShort>
          || std::is_same_v<Type, RawLong>
          || std::is_same_v<Type, RawFloat>
          || std::is_same_v<Type, RawRef>>>
  llvm::AllocaInst *createEntryBlockAlloca(llvm::Function *fun,
                                           const std::string &varName) {
    llvm::BasicBlock &entryBlock{fun->getEntryBlock()};
    llvm::IRBuilder irBuilder(&entryBlock, entryBlock.begin());

    llvm::Type *type{};
    // TODO: Treat references correctly
    if constexpr (std::is_same_v<Type, RawShort>) {
      type = llvm::Type::getInt16Ty(mCtx);
    } else if constexpr (std::is_same_v<Type, RawLong>) {
      type = llvm::Type::getInt32Ty(mCtx);
    } else if constexpr (std::is_same_v<Type, RawRef>) {
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

 public:

  explicit LLVMVisitor(llvm::StringRef moduleName)
      : mIrBuilder(mCtx), mModule(moduleName, mCtx) {}

  llvm::Value *visit(const AstNode &node);

  void print() {
    mModule.print(llvm::errs(), nullptr);
  }
};

template<> llvm::Value *
LLVMVisitor::visitImpl<RawScriptnameStatement>(const AstNode &node);

template<> llvm::Value *
LLVMVisitor::visitImpl<RawIdentifier>(const AstNode &node);

template<> llvm::Value *
LLVMVisitor::visitImpl<BlockStatement>(const AstNode &node);

template<> llvm::Value *
LLVMVisitor::visitImpl<IntegerLiteral>(const AstNode &node);

template<> llvm::Value *
LLVMVisitor::visitImpl<FloatLiteral>(const AstNode &node);

template<> llvm::Value *
LLVMVisitor::visitImpl<DeclarationStatement>(const AstNode &node);

template<> llvm::Value *
LLVMVisitor::visitImpl<SetStatement>(const AstNode &node);

template<> llvm::Value *
LLVMVisitor::visitImpl<StrPlus>(const AstNode &node);

template<> llvm::Value *
LLVMVisitor::visitImpl<StrDash>(const AstNode &node);

template<> llvm::Value *
LLVMVisitor::visitImpl<StrStar>(const AstNode &node);

template<> llvm::Value *
LLVMVisitor::visitImpl<StrSlash>(const AstNode &node);

} // namespace scripting

#endif // OPENOBLIVION_SCRIPTING_LLVM_HPP
