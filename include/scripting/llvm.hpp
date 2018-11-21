#ifndef OPENOBLIVION_SCRIPTING_LLVM_HPP
#define OPENOBLIVION_SCRIPTING_LLVM_HPP

#include "scripting/grammar.hpp"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <optional>

namespace scripting {

class LlvmVisitor {
 private:
  llvm::LLVMContext mCtx{};
  llvm::IRBuilder<> mIrBuilder;
  llvm::Module mModule;

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

 public:

  explicit LlvmVisitor(llvm::StringRef moduleName)
      : mIrBuilder(mCtx), mModule(moduleName, mCtx) {}

  llvm::Value *visit(const pegtl::parse_tree::node &node);

  void print() {
    mModule.print(llvm::errs(), nullptr);
  }
};

template<> llvm::Value *
LlvmVisitor::visitImpl<IntegerLiteral>(const pegtl::parse_tree::node &node);

template<> llvm::Value *
LlvmVisitor::visitImpl<FloatLiteral>(const pegtl::parse_tree::node &node);

template<> llvm::Value *
LlvmVisitor::visitImpl<StrPlus>(const pegtl::parse_tree::node &node);

template<> llvm::Value *
LlvmVisitor::visitImpl<SetStatement>(const pegtl::parse_tree::node &node);

template<> llvm::Value *
LlvmVisitor::visitImpl<RawScriptnameStatement>(const pegtl::parse_tree::node &node);

} // namespace scripting

#endif // OPENOBLIVION_SCRIPTING_LLVM_HPP
