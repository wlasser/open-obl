#include "scripting/llvm.hpp"

namespace scripting {

llvm::Value *LlvmVisitor::visit(const pegtl::parse_tree::node &node) {
  if (node.is_root()) {
    for (const auto &child : node.children) {
      llvm::Value *v = visit(*child);
      if (v != nullptr) v->print(llvm::errs());
    }
  }
  if (auto v = visitHelper<RawScriptnameStatement>(node)) return v;
  if (auto v = visitHelper<RawScriptname>(node)) return v;
  if (auto v = visitHelper<RawIdentifier>(node)) return v;
  if (auto v = visitHelper<BlockStatement>(node)) return v;
  if (auto v = visitHelper<StringLiteralContents>(node)) return v;
  if (auto v = visitHelper<IntegerLiteral>(node)) return v;
  if (auto v = visitHelper<RefLiteralContents>(node)) return v;
  if (auto v = visitHelper<FloatLiteral>(node)) return v;
  if (auto v = visitHelper<DeclarationStatement>(node)) return v;
  if (auto v = visitHelper<SetStatement>(node)) return v;
  if (auto v = visitHelper<RawShort>(node)) return v;
  if (auto v = visitHelper<RawLong>(node)) return v;
  if (auto v = visitHelper<RawRef>(node)) return v;
  if (auto v = visitHelper<MemberAccess>(node)) return v;
  if (auto v = visitHelper<StrPlus>(node)) return v;
  if (auto v = visitHelper<StrDash>(node)) return v;
  if (auto v = visitHelper<StrStar>(node)) return v;
  if (auto v = visitHelper<StrSlash>(node)) return v;
  if (auto v = visitHelper<StrLteq>(node)) return v;
  if (auto v = visitHelper<StrGteq>(node)) return v;
  if (auto v = visitHelper<StrLt>(node)) return v;
  if (auto v = visitHelper<StrGt>(node)) return v;
  if (auto v = visitHelper<StrEqeq>(node)) return v;
  if (auto v = visitHelper<StrNeq>(node)) return v;
  if (auto v = visitHelper<StrAnd>(node)) return v;
  if (auto v = visitHelper<StrOr>(node)) return v;

  return nullptr;
}

template<> llvm::Value *
LlvmVisitor::visitImpl<IntegerLiteral>(const pegtl::parse_tree::node &node) {
  const std::string sVal{node.content()};
  return llvm::ConstantInt::get(mCtx, llvm::APInt(32u, sVal, 10u));
}

template<> llvm::Value *
LlvmVisitor::visitImpl<FloatLiteral>(const pegtl::parse_tree::node &node) {
  const std::string sVal{node.content()};
  const llvm::APFloat fVal(llvm::APFloat::IEEEsingle(), sVal);
  return llvm::ConstantFP::get(mCtx, fVal);
}

template<> llvm::Value *
LlvmVisitor::visitImpl<StrPlus>(const pegtl::parse_tree::node &node) {
  if (node.children.size() != 2) return nullptr;
  llvm::Value *lhs{visit(*node.children[0])};
  llvm::Value *rhs{visit(*node.children[1])};
  if (!lhs || !rhs) return nullptr;

  return mIrBuilder.CreateAdd(lhs, rhs, "addtmp");
}

template<> llvm::Value *
LlvmVisitor::visitImpl<SetStatement>(const pegtl::parse_tree::node &node) {
  return visit(*node.children[1]);
}

template<> llvm::Value *
LlvmVisitor::visitImpl<RawScriptnameStatement>(const pegtl::parse_tree::node &node) {
  return nullptr;
}

} // namespace scripting
