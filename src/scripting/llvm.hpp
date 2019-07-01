#ifndef OPENOBL_SCRIPTING_LLVM_HPP
#define OPENOBL_SCRIPTING_LLVM_HPP

#include "ast.hpp"
#include "grammar.hpp"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <optional>

namespace oo {

class LLVMVisitor {
 private:
  llvm::LLVMContext &mCtx;
  llvm::IRBuilder<> mIrBuilder;
  llvm::Module *mModule{};
  llvm::StringMap<llvm::AllocaInst *> mNamedValues{};
  llvm::StringMap<llvm::GlobalVariable *> mGlobals{};
  std::optional<uint32_t> mCalleeRef{};

  friend class ScriptEngineBase;

  template<class NodeType> llvm::Value *visitImpl(const AstNode &/*node*/) {
    return nullptr;
  }

  /// Create an alloca instruction in the entry block of the function.
  /// Use this for mutable local variables so that the mem2reg optimization pass
  /// can find them.
  llvm::AllocaInst *
  createAlloca(llvm::Function *fun, llvm::StringRef name, llvm::Type *type);

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
  explicit LLVMVisitor(llvm::Module *module,
                       llvm::LLVMContext &ctx,
                       std::optional<decltype(mIrBuilder)> irBuilder = std::nullopt,
                       std::optional<uint32_t> calleeRef = std::nullopt);

  llvm::Value *visit(const AstNode &node);
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
LLVMVisitor::visitImpl<grammar::RefLiteralContents>(const AstNode &node);

template<> llvm::Value *
LLVMVisitor::visitImpl<grammar::DeclarationStatement>(const AstNode &node);

template<> llvm::Value *
LLVMVisitor::visitImpl<grammar::SetStatement>(const AstNode &node);

template<> llvm::Value *
LLVMVisitor::visitImpl<grammar::ReturnStatement>(const AstNode &node);

template<> llvm::Value *
LLVMVisitor::visitImpl<grammar::IfStatement>(const AstNode &node);

template<> llvm::Value *
LLVMVisitor::visitImpl<grammar::CallStatement>(const AstNode &node);

template<> llvm::Value *
LLVMVisitor::visitImpl<grammar::RawCall>(const AstNode &node);

template<> llvm::Value *
LLVMVisitor::visitImpl<grammar::UnaryOperator>(const AstNode &node);

template<> llvm::Value *
LLVMVisitor::visitImpl<grammar::BinaryOperator>(const AstNode &node);

} // namespace oo

#endif // OPENOBL_SCRIPTING_LLVM_HPP
