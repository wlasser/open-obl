#include "scripting/console_engine.hpp"
#include <gsl/gsl>
#include <cstdio>
#include <stdexcept>

int ConsoleFunc(int x) {
  printf("%d\n", x);
  return 0;
}

namespace oo {

ConsoleEngine::ConsoleEngine() : ScriptEngineBase() {
  addExternalFun<decltype(ConsoleFunc)>("ConsoleFunc");
}

std::unique_ptr<llvm::Module>
ConsoleEngine::compileStatement(const AstNode &node) {
  if (!node.is_root() || node.children.empty()) {
    // TODO: Cannot compile a partial AST, throw
    return nullptr;
  }

  auto module{makeModule("__console_statement")};
  addExternalFunsToModule(module.get());

  // The AST contains a single statement, which needs to be wrapped in an
  // anonymous function.
  auto &ctx{getContext()};
  llvm::IRBuilder irBuilder(ctx);

  auto *funType{llvm::FunctionType::get(llvm::Type::getVoidTy(ctx), {})};
  auto *fun{llvm::Function::Create(funType,
                                   llvm::Function::ExternalLinkage,
                                   "__console_statement_fun",
                                   module.get())};
  auto *bb{llvm::BasicBlock::Create(ctx, "entry", fun)};
  irBuilder.SetInsertPoint(bb);

  auto visitor{makeVisitor(module.get(), irBuilder)};
  visitor.visit(node);

  irBuilder.CreateRetVoid();

  return module;
}

void ConsoleEngine::execute(std::string_view statement) {
  pegtl::memory_input in(statement, "");
  const auto root{oo::parseStatement(in)};
  if (!root) {
    throw std::runtime_error("Syntax error: Failed to parse statement");
  }

  auto module{compileStatement(*root)};
  if (!module) {
    throw std::runtime_error("Semantic error: Failed to compile statement");
  }

  auto key{jitModule(std::move(module))};
  const auto moduleGuard = gsl::finally([&]() {
    getJit()->removeModule(key);
  });

  auto symbol{getJit()->findSymbol("__console_statement_fun")};
  if (!symbol) {
    throw std::runtime_error("Failed to find statement, JIT must have failed");
  }

  auto addrOrErr{symbol.getAddress()};
  if (llvm::Error err = addrOrErr.takeError()) {
    llvm::handleAllErrors(std::move(err), [](const llvm::ErrorInfoBase &e) {
      scriptingLogger()->warn("JIT error: {}", e.message());
    });
    throw std::runtime_error("Failed to find statement address");
  }

  using func_t = void (*)();
  auto addr{reinterpret_cast<std::uintptr_t>(*addrOrErr)};
  auto func{reinterpret_cast<func_t>(addr)};

  func();
}

} // namespace oo