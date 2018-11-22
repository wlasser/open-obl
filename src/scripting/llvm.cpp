#include "scripting/llvm.hpp"
#include <llvm/IR/Verifier.h>

// There is a bug in CLion (https://youtrack.jetbrains.com/issue/CPP-11511 is a
// likely candidate) which keeps resulting in a `condition is always true`
// warning when comparing llvm::Type*. It marks subsequent code in the function
// as unreachable, which is very annoying.
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCDFAInspection"

namespace scripting {

llvm::Value *LlvmVisitor::visit(const pegtl::parse_tree::node &node) {
  if (node.is_root()) {
    for (const auto &child : node.children) {
      visit(*child);
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
  if (auto v = visitHelper<RawFloat>(node)) return v;
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
LlvmVisitor::visitImpl<RawIdentifier>(const pegtl::parse_tree::node &node) {
  auto it{mNamedValues.find(node.content())};
  if (it == mNamedValues.end()) {
    // TODO: Variable does not exist
    return nullptr;
  }
  return mIrBuilder.CreateLoad(it->second);
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

  llvm::Type *lhsType{lhs->getType()};
  llvm::Type *rhsType{rhs->getType()};

  llvm::Type *int16Type{llvm::Type::getInt16Ty(mCtx)};
  llvm::Type *int32Type{llvm::Type::getInt32Ty(mCtx)};
  llvm::Type *floatType{llvm::Type::getFloatTy(mCtx)};

  if (lhsType == rhsType) {
    return mIrBuilder.CreateAdd(lhs, rhs, "addtmp");
  }

  // As in C++, if either operand is float then the other is converted to float
  if (lhsType == floatType && rhsType->isIntegerTy()) {
    llvm::Value *rhsProm{mIrBuilder.CreateSIToFP(rhs, lhsType)};
    return mIrBuilder.CreateFAdd(lhs, rhsProm);
  } else if (lhsType->isIntegerTy() && rhsType == floatType) {
    llvm::Value *lhsProm{mIrBuilder.CreateSIToFP(lhs, rhsType)};
    return mIrBuilder.CreateFAdd(lhsProm, rhs);
  }

  // As in C++, promote i16 -> i32
  if (lhsType == int32Type && rhsType == int16Type) {
    llvm::Value *rhsProm{mIrBuilder.CreateZExt(rhs, lhsType)};
    return mIrBuilder.CreateAdd(lhs, rhsProm);
  } else if (lhsType == int16Type && rhsType == int32Type) {
    llvm::Value *lhsProm{mIrBuilder.CreateZExt(lhs, rhsType)};
    return mIrBuilder.CreateAdd(lhsProm, rhs);
  }

  return nullptr;
}

template<> llvm::Value *
LlvmVisitor::visitImpl<SetStatement>(const pegtl::parse_tree::node &node) {
  llvm::Value *src{visit(*node.children[1])};
  if (src == nullptr) {
    // TODO: RHS is ill-formed
    return nullptr;
  }

  auto destIt{mNamedValues.find(node.children[0]->content())};
  if (destIt == mNamedValues.end()) {
    // TODO: Variable does not exist
    return nullptr;
  }

  llvm::AllocaInst *dest{destIt->second};

  // Storing a value into itself does nothing
  if (src == dest) return nullptr;

  llvm::Type *srcType{src->getType()};
  llvm::Type *destType{dest->getType()->getPointerElementType()};

  // No conversions required
  if (srcType == destType) {
    return mIrBuilder.CreateStore(src, dest);
  }

  // Some kind of conversion is needed to match srcType to destType before the
  // store instruction.
  // TODO: Handle reference types correctly

  // Zero extend i16 into i32 and trunc i32 into i16.
  if (srcType->isIntegerTy() && destType->isIntegerTy()) {
    llvm::Value *converted{mIrBuilder.CreateZExtOrTrunc(src, destType)};
    return mIrBuilder.CreateStore(converted, dest);
  }

  // Round float into i32 or i16
  if (destType->isIntegerTy()) {
    if (srcType == llvm::Type::getFloatTy(mCtx)) {
      llvm::Value *fptosi{mIrBuilder.CreateFPToSI(src, destType)};
      return mIrBuilder.CreateStore(fptosi, dest);
    }
  }

  // Promote i16 or i32 into float
  if (destType->isFloatTy()) {
    if (srcType->isIntegerTy()) {
      llvm::Value *sitofp{mIrBuilder.CreateSIToFP(src, destType)};
      return mIrBuilder.CreateStore(sitofp, dest);
    }
  }

  return nullptr;
}

template<> llvm::Value *
LlvmVisitor::visitImpl<DeclarationStatement>(const pegtl::parse_tree::node &node) {
  std::string varName{node.children[1]->content()};
  llvm::Function *fun{mIrBuilder.GetInsertBlock()->getParent()};

  // Create an alloca instruction and default value for the variable
  llvm::AllocaInst *alloca{};
  llvm::Value *init{};

  if (node.children[0]->is<RawShort>()) {
    alloca = createEntryBlockAlloca<RawShort>(fun, varName);
    init = llvm::ConstantInt::get(mCtx, llvm::APInt(16u, 0));
  } else if (node.children[0]->is<RawLong>()) {
    alloca = createEntryBlockAlloca<RawLong>(fun, varName);
    init = llvm::ConstantInt::get(mCtx, llvm::APInt(32u, 0));
  } else if (node.children[0]->is<RawRef>()) {
    // TODO: Handle reference variables correctly
    alloca = createEntryBlockAlloca<RawRef>(fun, varName);
    init = llvm::ConstantInt::get(mCtx, llvm::APInt(32u, 0));
  } else {
    alloca = createEntryBlockAlloca<RawFloat>(fun, varName);
    init = llvm::ConstantFP::get(mCtx,
                                 llvm::APFloat(llvm::APFloat::IEEEsingle(), 0));
  }

  // Keep track of it in the local function table
  mNamedValues[varName] = alloca;

  // Store an initial value
  mIrBuilder.CreateStore(init, alloca);

  return alloca;
}

template<> llvm::Value *
LlvmVisitor::visitImpl<RawScriptnameStatement>(const pegtl::parse_tree::node &node) {
  return nullptr;
}

template<> llvm::Value *
LlvmVisitor::visitImpl<BlockStatement>(const pegtl::parse_tree::node &node) {
  if (node.children.empty()) return nullptr;

  auto blockStart = node.children.begin() + 1;
  std::string blockName{node.children[0]->content()};

  // TODO: Do more sophisticated name resolution by talking to the GUI
  if (node.children.size() > 1 && node.children[1]->is<IntegerLiteral>()) {
    blockName = node.children[0]->content() + node.children[1]->content();
    ++blockStart;
  }

  auto *funType{llvm::FunctionType::get(llvm::Type::getVoidTy(mCtx), false)};
  auto *fun{llvm::Function::Create(funType,
                                   llvm::Function::ExternalLinkage,
                                   blockName,
                                   &mModule)};
  auto *bb{llvm::BasicBlock::Create(mCtx, "entry", fun)};
  mIrBuilder.SetInsertPoint(bb);

  for (auto it{blockStart}; it != node.children.end(); ++it) {
    const auto &statement{*it};
    visit(*statement);
  }

  mIrBuilder.CreateRetVoid();
  llvm::verifyFunction(*fun);

  return fun;
}

} // namespace scripting

#pragma clang diagnostic pop