#include "scripting/llvm.hpp"
#include <llvm/IR/Verifier.h>

// There is a bug in CLion (https://youtrack.jetbrains.com/issue/CPP-11511 is a
// likely candidate) which keeps resulting in a `condition is always true`
// warning when comparing llvm::Type*. It marks subsequent code in the function
// as unreachable, which is very annoying.
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCDFAInspection"

namespace oo {

llvm::Value *LLVMVisitor::visit(const AstNode &node) {
  if (node.is_root()) {
    for (const auto &child : node.children) {
      visit(*child);
    }
    return nullptr;
  }
  if (auto v = visitHelper<grammar::RawScriptnameStatement>(node)) return v;
  if (auto v = visitHelper<grammar::RawScriptname>(node)) return v;
  if (auto v = visitHelper<grammar::RawIdentifier>(node)) return v;
  if (auto v = visitHelper<grammar::BlockStatement>(node)) return v;
  if (auto v = visitHelper<grammar::StringLiteralContents>(node)) return v;
  if (auto v = visitHelper<grammar::IntegerLiteral>(node)) return v;
  if (auto v = visitHelper<grammar::RefLiteralContents>(node)) return v;
  if (auto v = visitHelper<grammar::FloatLiteral>(node)) return v;
  if (auto v = visitHelper<grammar::DeclarationStatement>(node)) return v;
  if (auto v = visitHelper<grammar::SetStatement>(node)) return v;
  if (auto v = visitHelper<grammar::ReturnStatement>(node)) return v;
  if (auto v = visitHelper<grammar::RawShort>(node)) return v;
  if (auto v = visitHelper<grammar::RawLong>(node)) return v;
  if (auto v = visitHelper<grammar::RawFloat>(node)) return v;
  if (auto v = visitHelper<grammar::RawRef>(node)) return v;
  if (auto v = visitHelper<grammar::RawMemberAccess>(node)) return v;
  if (auto v = visitHelper<grammar::RawCall>(node)) return v;
  if (auto v = visitHelper<grammar::BinaryOperator>(node)) return v;
  if (auto v = visitHelper<grammar::UnaryOperator>(node)) return v;

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
LLVMVisitor::visitImpl<grammar::RawScriptnameStatement>(const AstNode &node) {
  return nullptr;
}

template<> llvm::Value *
LLVMVisitor::visitImpl<grammar::RawIdentifier>(const AstNode &node) {
  if (auto it{mNamedValues.find(node.content())}; it != mNamedValues.end()) {
    return mIrBuilder.CreateLoad(it->second);
  }

  if (auto it{mGlobals.find(node.content())}; it != mGlobals.end()) {
    return mIrBuilder.CreateLoad(it->second);
  }

  // TODO: Variable does not exist
  return nullptr;
}

template<> llvm::Value *
LLVMVisitor::visitImpl<grammar::BlockStatement>(const AstNode &node) {
  if (node.children.empty()) return nullptr;

  auto blockStart{node.children.begin() + 1};
  std::string blockName{node.children[0]->content()};

  // TODO: Do more sophisticated name resolution by talking to the GUI
  if (node.children.size() > 1
      && node.children[1]->is<grammar::IntegerLiteral>()) {
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

  auto namedValues{mNamedValues};

  for (auto it{blockStart}; it != node.children.end(); ++it) {
    const auto &statement{*it};
    visit(*statement);
  }

  mNamedValues = namedValues;

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
LLVMVisitor::visitImpl<grammar::IntegerLiteral>(const AstNode &node) {
  const std::string sVal{node.content()};
  return llvm::ConstantInt::get(mCtx, llvm::APInt(32u, sVal, 10u));
}

template<> llvm::Value *
LLVMVisitor::visitImpl<grammar::FloatLiteral>(const AstNode &node) {
  const std::string sVal{node.content()};
  const llvm::APFloat fVal(llvm::APFloat::IEEEsingle(), sVal);
  return llvm::ConstantFP::get(mCtx, fVal);
}

template<> llvm::Value *
LLVMVisitor::visitImpl<grammar::DeclarationStatement>(const AstNode &node) {
  std::string varName{node.children[1]->content()};
  llvm::BasicBlock *bb{mIrBuilder.GetInsertBlock()};

  // Global variable
  if (!bb) {
    const auto linkage{llvm::GlobalVariable::ExternalLinkage};
    llvm::Constant *init{};
    llvm::Type *type{};

    const auto &declType{node.children[0]};
    const auto &declName{node.children[1]->content()};

    if (declType->is<grammar::RawShort>()) {
      type = llvm::Type::getInt16Ty(mCtx);
      init = llvm::ConstantInt::get(type, 0, true);
    } else if (declType->is<grammar::RawLong>()) {
      type = llvm::Type::getInt32Ty(mCtx);
      init = llvm::ConstantInt::get(type, 0, true);
    } else if (declType->is<grammar::RawRef>()) {
      type = llvm::Type::getInt32Ty(mCtx);
      init = llvm::ConstantInt::get(type, 0, true);
    } else if (declType->is<grammar::RawFloat>()) {
      type = llvm::Type::getFloatTy(mCtx);
      init = llvm::ConstantFP::get(type, 0.0);
    }

    // Create a global variable initialized to zero
    mModule.getOrInsertGlobal(declName, type);
    llvm::GlobalVariable *glob{mModule.getNamedGlobal(declName)};
    glob->setInitializer(init);
    glob->setDSOLocal(true);
    glob->setLinkage(linkage);

    mGlobals[declName] = glob;

    return glob;
  } else {
    llvm::Function *fun{bb->getParent()};

    // Create an alloca instruction and default value for the variable
    llvm::AllocaInst *alloca{};
    llvm::Value *init{};

    // TODO: Handle reference variables correctly
    if (node.children[0]->is<grammar::RawShort>()) {
      alloca = createEntryBlockAlloca<grammar::RawShort>(fun, varName);
      init = llvm::ConstantInt::get(mCtx, llvm::APInt(16u, 0));
    } else if (node.children[0]->is<grammar::RawLong>()) {
      alloca = createEntryBlockAlloca<grammar::RawLong>(fun, varName);
      init = llvm::ConstantInt::get(mCtx, llvm::APInt(32u, 0));
    } else if (node.children[0]->is<grammar::RawRef>()) {
      alloca = createEntryBlockAlloca<grammar::RawRef>(fun, varName);
      init = llvm::ConstantInt::get(mCtx, llvm::APInt(32u, 0));
    } else {
      alloca = createEntryBlockAlloca<grammar::RawFloat>(fun, varName);
      init = llvm::ConstantFP::get(llvm::Type::getFloatTy(mCtx), 0.0);
    }

    // Keep track of it in the local function table
    mNamedValues[varName] = alloca;

    // Store an initial value
    mIrBuilder.CreateStore(init, alloca);

    return alloca;
  }
}

template<> llvm::Value *
LLVMVisitor::visitImpl<grammar::SetStatement>(const AstNode &node) {
  llvm::Value *src{visit(*node.children[1])};
  if (src == nullptr) {
    // TODO: RHS is ill-formed
    return nullptr;
  }

  llvm::Value *dest = [&]() -> llvm::Value * {
    auto destName{node.children[0]->content()};
    if (auto it{mNamedValues.find(destName)}; it != mNamedValues.end()) {
      return it->second;
    }
    if (auto it{mGlobals.find(destName)}; it != mGlobals.end()) {
      return it->second;
    }
    return nullptr;
  }();

  if (!dest) {
    // TODO: Variable does not exist
    return nullptr;
  }

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
LLVMVisitor::visitImpl<grammar::ReturnStatement>(const AstNode &node) {
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
LLVMVisitor::visitImpl<grammar::RawCall>(const AstNode &node) {
  std::string funName{node.getValue()};
  llvm::Function *proto{};

  if (auto it{mFunctions.find(funName)}; it != mFunctions.end()) {
    proto = it->second;
  } else {
    // TODO: No function with that name exists
    return nullptr;
  }

  if (node.children.size() != proto->arg_size()) {
    // TODO: Incorrect number of arguments
    return nullptr;
  }

  llvm::SmallVector<llvm::Value *, 4> args{};
  for (std::size_t i = 0; i < node.children.size(); ++i) {
    const auto &child{node.children[i]};
    const auto &arg{proto->arg_begin() + i};

    args.emplace_back(visit(*child));

    if (args.back()->getType() != arg->getType()) {
      // TODO: Argument mismatch, throw or convert
      return nullptr;
    }
  }

  return mIrBuilder.CreateCall(proto, args);
}

template<> llvm::Value *
LLVMVisitor::visitImpl<grammar::BinaryOperator>(const AstNode &node) {
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
LLVMVisitor::visitImpl<grammar::UnaryOperator>(const AstNode &node) {
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

} // namespace oo

#pragma clang diagnostic pop
