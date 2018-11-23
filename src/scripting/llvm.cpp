#include "scripting/llvm.hpp"
#include <llvm/IR/Verifier.h>

// There is a bug in CLion (https://youtrack.jetbrains.com/issue/CPP-11511 is a
// likely candidate) which keeps resulting in a `condition is always true`
// warning when comparing llvm::Type*. It marks subsequent code in the function
// as unreachable, which is very annoying.
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCDFAInspection"

namespace scripting {

llvm::Value *LLVMVisitor::visit(const AstNode &node) {
  if (node.is_root()) {
    for (const auto &child : node.children) {
      visit(*child);
    }
    return nullptr;
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
  if (auto v = visitHelper<ReturnStatement>(node)) return v;
  if (auto v = visitHelper<RawShort>(node)) return v;
  if (auto v = visitHelper<RawLong>(node)) return v;
  if (auto v = visitHelper<RawFloat>(node)) return v;
  if (auto v = visitHelper<RawRef>(node)) return v;
  if (auto v = visitHelper<MemberAccess>(node)) return v;
  if (auto v = visitHelper<BinaryOperator>(node)) return v;
  if (auto v = visitHelper<UnaryOperator>(node)) return v;

  return nullptr;
}

[[nodiscard]] std::pair<llvm::Value *, llvm::Value *>
LLVMVisitor::promoteArithmeticOperands(llvm::Value *lhs, llvm::Value *rhs) {
  llvm::Type *int1Type{llvm::Type::getInt1Ty(mCtx)};
  llvm::Type *int16Type{llvm::Type::getInt16Ty(mCtx)};
  llvm::Type *int32Type{llvm::Type::getInt32Ty(mCtx)};
  llvm::Type *floatType{llvm::Type::getFloatTy(mCtx)};

  llvm::Type *lhsType{lhs->getType()};
  llvm::Type *rhsType{rhs->getType()};

  if (lhsType == rhsType) return {lhs, rhs};

  // If either operand is float then the other is converted to float
  if (lhsType == floatType) {
    if (rhsType == int1Type) {
      llvm::Value *rhsProm{mIrBuilder.CreateUIToFP(rhs, lhsType)};
      return {lhs, rhsProm};
    } else if (rhsType->isIntegerTy()) {
      llvm::Value *rhsProm{mIrBuilder.CreateSIToFP(rhs, lhsType)};
      return {lhs, rhsProm};
    }
  } else if (rhsType == floatType) {
    if (lhsType == int1Type) {
      llvm::Value *lhsProm{mIrBuilder.CreateUIToFP(lhs, rhsType)};
      return {lhsProm, rhs};
    } else if (lhsType->isIntegerTy()) {
      llvm::Value *lhsProm{mIrBuilder.CreateSIToFP(lhs, rhsType)};
      return {lhsProm, rhs};
    }
  }

  // Promote i1 -> i16 or i32
  if (lhsType->isIntegerTy() && rhsType == int1Type) {
    llvm::Value *rhsProm{mIrBuilder.CreateZExt(rhs, lhsType)};
    return {lhs, rhsProm};
  } else if (lhsType == int1Type && rhsType->isIntegerTy()) {
    llvm::Value *lhsProm{mIrBuilder.CreateZExt(lhs, rhsType)};
    return {lhsProm, rhs};
  }

  // Promote i16 -> i32
  if (lhsType == int32Type && rhsType == int16Type) {
    llvm::Value *rhsProm{mIrBuilder.CreateSExt(rhs, lhsType)};
    return {lhs, rhsProm};
  } else if (lhsType == int16Type && rhsType == int32Type) {
    llvm::Value *lhsProm{mIrBuilder.CreateSExt(lhs, rhsType)};
    return {lhsProm, rhs};
  }

  return {lhs, rhs};
}

[[nodiscard]] llvm::Value *LLVMVisitor::convertToBool(llvm::Value *lhs) {
  llvm::Type *lhsType{lhs->getType()};

  if (lhsType->isIntegerTy(1)) {
    return lhs;
  }

  if (lhsType->isIntegerTy()) {
    auto zero = llvm::ConstantInt::get(lhsType, 0u, true);
    return mIrBuilder.CreateICmpNE(lhs, zero);
  }

  if (lhsType->isFloatTy()) {
    auto zero = llvm::ConstantFP::get(llvm::Type::getFloatTy(mCtx), 0.0);
    return mIrBuilder.CreateFCmpUNE(lhs, zero);
  }

  return lhs;
}

template<> llvm::Value *
LLVMVisitor::visitImpl<RawScriptnameStatement>(const AstNode &node) {
  return nullptr;
}

template<> llvm::Value *
LLVMVisitor::visitImpl<RawIdentifier>(const AstNode &node) {
  auto it{mNamedValues.find(node.content())};
  if (it == mNamedValues.end()) {
    // TODO: Variable does not exist
    return nullptr;
  }
  return mIrBuilder.CreateLoad(it->second);
}

template<> llvm::Value *
LLVMVisitor::visitImpl<BlockStatement>(const AstNode &node) {
  if (node.children.empty()) return nullptr;

  auto blockStart{node.children.begin() + 1};
  std::string blockName{node.children[0]->content()};

  // TODO: Do more sophisticated name resolution by talking to the GUI
  if (node.children.size() > 1 && node.children[1]->is<IntegerLiteral>()) {
    blockName = node.children[0]->content() + node.children[1]->content();
    ++blockStart;
  }

  llvm::FunctionType *funType = [&]() {
    if (blockName == "TestLong") {
      return llvm::FunctionType::get(llvm::Type::getInt32Ty(mCtx), false);
    } else if (blockName == "TestShort") {
      return llvm::FunctionType::get(llvm::Type::getInt16Ty(mCtx), false);
    } else if (blockName == "TestFloat") {
      return llvm::FunctionType::get(llvm::Type::getFloatTy(mCtx), false);
    } else {
      return llvm::FunctionType::get(llvm::Type::getVoidTy(mCtx), false);
    }
  }();

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

  if (funType->getReturnType()->isVoidTy()) {
    mIrBuilder.CreateRetVoid();
  } else if (funType->getReturnType()->isIntegerTy(16u)) {
    llvm::Value *zero{llvm::ConstantInt::get(mCtx, llvm::APInt(16u, 0))};
    mIrBuilder.CreateRet(zero);
  } else if (funType->getReturnType()->isIntegerTy(32u)) {
    llvm::Value *zero{llvm::ConstantInt::get(mCtx, llvm::APInt(32u, 0))};
    mIrBuilder.CreateRet(zero);
  } else if (funType->getReturnType()->isFloatTy()) {
    llvm::Value *zero{llvm::ConstantFP::get(llvm::Type::getFloatTy(mCtx), 0.0)};
    mIrBuilder.CreateRet(zero);
  }

  llvm::verifyFunction(*fun);

  mPassManager.run(*fun, mAnalysisManager);

  return fun;
}

template<> llvm::Value *
LLVMVisitor::visitImpl<IntegerLiteral>(const AstNode &node) {
  const std::string sVal{node.content()};
  return llvm::ConstantInt::get(mCtx, llvm::APInt(32u, sVal, 10u));
}

template<> llvm::Value *
LLVMVisitor::visitImpl<FloatLiteral>(const AstNode &node) {
  const std::string sVal{node.content()};
  const llvm::APFloat fVal(llvm::APFloat::IEEEsingle(), sVal);
  return llvm::ConstantFP::get(mCtx, fVal);
}

template<> llvm::Value *
LLVMVisitor::visitImpl<DeclarationStatement>(const AstNode &node) {
  std::string varName{node.children[1]->content()};
  llvm::Function *fun{mIrBuilder.GetInsertBlock()->getParent()};

  // Create an alloca instruction and default value for the variable
  llvm::AllocaInst *alloca{};
  llvm::Value *init{};

  // TODO: Handle reference variables correctly
  if (node.children[0]->is<RawShort>()) {
    alloca = createEntryBlockAlloca<RawShort>(fun, varName);
    init = llvm::ConstantInt::get(mCtx, llvm::APInt(16u, 0));
  } else if (node.children[0]->is<RawLong>()) {
    alloca = createEntryBlockAlloca<RawLong>(fun, varName);
    init = llvm::ConstantInt::get(mCtx, llvm::APInt(32u, 0));
  } else if (node.children[0]->is<RawRef>()) {
    alloca = createEntryBlockAlloca<RawRef>(fun, varName);
    init = llvm::ConstantInt::get(mCtx, llvm::APInt(32u, 0));
  } else {
    alloca = createEntryBlockAlloca<RawFloat>(fun, varName);
    init = llvm::ConstantFP::get(llvm::Type::getFloatTy(mCtx), 0.0);
  }

  // Keep track of it in the local function table
  mNamedValues[varName] = alloca;

  // Store an initial value
  mIrBuilder.CreateStore(init, alloca);

  return alloca;
}

template<> llvm::Value *
LLVMVisitor::visitImpl<SetStatement>(const AstNode &node) {
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

  // Zero extend i1 into integer types.
  if (srcType->isIntegerTy(1) && destType->isIntegerTy()) {
    llvm::Value *newSrc{mIrBuilder.CreateZExt(src, destType)};
    return mIrBuilder.CreateStore(newSrc, dest);
  }

  // Unsigned convert i1 into float.
  if (srcType->isIntegerTy(1) && destType->isFloatTy()) {
    llvm::Value *newSrc{mIrBuilder.CreateUIToFP(src, destType)};
    return mIrBuilder.CreateStore(newSrc, dest);
  }

  // Sign extend i16 into i32 and trunc i32 into i16.
  if (srcType->isIntegerTy() && destType->isIntegerTy()) {
    llvm::Value *newSrc{mIrBuilder.CreateSExtOrTrunc(src, destType)};
    return mIrBuilder.CreateStore(newSrc, dest);
  }

  // Round float into i32 or i16.
  if (destType->isIntegerTy() && srcType == llvm::Type::getFloatTy(mCtx)) {
    llvm::Value *newSrc{mIrBuilder.CreateFPToSI(src, destType)};
    return mIrBuilder.CreateStore(newSrc, dest);
  }

  // Signed convert i16 or i32 into float.
  if (destType->isFloatTy() && srcType->isIntegerTy()) {
    llvm::Value *newSrc{mIrBuilder.CreateSIToFP(src, destType)};
    return mIrBuilder.CreateStore(newSrc, dest);
  }

  return nullptr;
}

template<> llvm::Value *
LLVMVisitor::visitImpl<ReturnStatement>(const AstNode &node) {
  if (node.children.empty()) {
    return mIrBuilder.CreateRetVoid();
  }

  llvm::Value *val{visit(*node.children[0])};
  if (!val) return nullptr;

  llvm::Function *fun{mIrBuilder.GetInsertBlock()->getParent()};
  if (val->getType() != fun->getReturnType()) {
    // TODO: Wrong return type, attempt to convert otherwise fail
    return nullptr;
  }

  return mIrBuilder.CreateRet(val);
}

template<> llvm::Value *
LLVMVisitor::visitImpl<BinaryOperator>(const AstNode &node) {
  if (node.children.size() != 2) return nullptr;
  llvm::Value *lhs{visit(*node.children[0])};
  llvm::Value *rhs{visit(*node.children[1])};
  if (!lhs || !rhs) return nullptr;

  std::string_view op{node.getValue()};

  // Logical operators convert differently to arithmetic operators; instead of
  // converting to a common type, both arguments are converted to bool. This is
  // different to C++, which short-circuits and thus will not convert both
  // arguments if it is not necessary to evaluate the expression.
  if (op == "&&" || op == "||") {
    auto newLhs{convertToBool(lhs)};
    auto newRhs{convertToBool(rhs)};
    if (op == "&&") return mIrBuilder.CreateAnd(newLhs, newRhs);
    if (op == "||") return mIrBuilder.CreateOr(newLhs, newRhs);
  }

  auto[newLhs, newRhs]{promoteArithmeticOperands(lhs, rhs)};

  if (newLhs->getType()->isFloatTy()) {
    if (op == "+") return mIrBuilder.CreateFAdd(newLhs, newRhs);
    else if (op == "-") return mIrBuilder.CreateFSub(newLhs, newRhs);
    else if (op == "*") return mIrBuilder.CreateFMul(newLhs, newRhs);
    else if (op == "/") return mIrBuilder.CreateFDiv(newLhs, newRhs);
    else if (op == "<=") return mIrBuilder.CreateFCmpOLE(newLhs, newRhs);
    else if (op == ">=") return mIrBuilder.CreateFCmpOGE(newLhs, newRhs);
    else if (op == "<") return mIrBuilder.CreateFCmpOLT(newLhs, newRhs);
    else if (op == ">") return mIrBuilder.CreateFCmpOGT(newLhs, newRhs);
    else if (op == "==") return mIrBuilder.CreateFCmpOEQ(newLhs, newRhs);
    else if (op == "!=") return mIrBuilder.CreateFCmpONE(newLhs, newRhs);
  } else {
    if (op == "+") return mIrBuilder.CreateAdd(newLhs, newRhs);
    else if (op == "-") return mIrBuilder.CreateSub(newLhs, newRhs);
    else if (op == "*") return mIrBuilder.CreateMul(newLhs, newRhs);
    else if (op == "/") return mIrBuilder.CreateSDiv(newLhs, newRhs);
    else if (op == "<=") return mIrBuilder.CreateICmpSLE(newLhs, newRhs);
    else if (op == ">=") return mIrBuilder.CreateICmpSGE(newLhs, newRhs);
    else if (op == "<") return mIrBuilder.CreateICmpSLT(newLhs, newRhs);
    else if (op == ">") return mIrBuilder.CreateICmpSGT(newLhs, newRhs);
    else if (op == "==") return mIrBuilder.CreateICmpEQ(newLhs, newRhs);
    else if (op == "!=") return mIrBuilder.CreateICmpNE(newLhs, newRhs);
  }

  return nullptr;
}

template<> llvm::Value *
LLVMVisitor::visitImpl<UnaryOperator>(const AstNode &node) {
  if (node.children.size() != 1) return nullptr;
  llvm::Value *rhs{visit(*node.children[0])};
  if (!rhs) return nullptr;

  std::string_view op{node.getValue()};
  if (op == "+") {
    return rhs;
  } else if (op == "-") {
    if (rhs->getType()->isFloatTy()) {
      return mIrBuilder.CreateFNeg(rhs);
    }
    return mIrBuilder.CreateNeg(rhs);
  }

  return nullptr;
}

} // namespace scripting

#pragma clang diagnostic pop
