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
  llvm::Value *visitImpl(const pegtl::parse_tree::node &node) {
    return nullptr;
  }

  template<class NodeType>
  auto visitHelper(const pegtl::parse_tree::node &node)
  -> decltype(visitImpl<NodeType>(node)) {
    if (node.is<NodeType>()) return visitImpl<NodeType>(node);
    return nullptr;
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
    // IRBuilder to start of function entry block
    llvm::IRBuilder<> irBuilder(&fun->getEntryBlock(),
                                fun->getEntryBlock().begin());
    llvm::Type *type{};
    if constexpr (std::is_same_v<Type, RawShort>) {
      type = llvm::Type::getInt16Ty(mCtx);
    } else if constexpr (std::is_same_v<Type, RawLong>) {
      type = llvm::Type::getInt32Ty(mCtx);
    } else if constexpr (std::is_same_v<Type, RawRef>) {
      // TODO: Treat references correctly
      type = llvm::Type::getInt32Ty(mCtx);
    } else {
      type = llvm::Type::getFloatTy(mCtx);
    }
    return irBuilder.CreateAlloca(type, nullptr, varName);
  }

 public:

  explicit LLVMVisitor(llvm::StringRef moduleName)
      : mIrBuilder(mCtx), mModule(moduleName, mCtx) {}

  llvm::Value *visit(const pegtl::parse_tree::node &node);

  void print() {
    mModule.print(llvm::errs(), nullptr);
  }
};

template<> llvm::Value *
LLVMVisitor::visitImpl<RawScriptnameStatement>(const pegtl::parse_tree::node &node);

template<> llvm::Value *
LLVMVisitor::visitImpl<RawIdentifier>(const pegtl::parse_tree::node &node);

template<> llvm::Value *
LLVMVisitor::visitImpl<BlockStatement>(const pegtl::parse_tree::node &node);

template<> llvm::Value *
LLVMVisitor::visitImpl<IntegerLiteral>(const pegtl::parse_tree::node &node);

template<> llvm::Value *
LLVMVisitor::visitImpl<FloatLiteral>(const pegtl::parse_tree::node &node);

template<> llvm::Value *
LLVMVisitor::visitImpl<DeclarationStatement>(const pegtl::parse_tree::node &node);

template<> llvm::Value *
LLVMVisitor::visitImpl<SetStatement>(const pegtl::parse_tree::node &node);

template<> llvm::Value *
LLVMVisitor::visitImpl<StrPlus>(const pegtl::parse_tree::node &node);

} // namespace scripting

#endif // OPENOBLIVION_SCRIPTING_LLVM_HPP
