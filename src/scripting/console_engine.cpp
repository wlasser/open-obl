#include "ast.hpp"
#include "scripting/console_engine.hpp"
#include "jit.hpp"
#include "llvm.hpp"
#include "scripting/logging.hpp"
#include <gsl/gsl>
#include <stdexcept>

namespace oo {

class ConsoleEngine::Impl {
 private:
  nostdx::propagate_const<ConsoleEngine *> mParent;

 public:
  [[nodiscard]] std::unique_ptr<llvm::Module>
  compileStatement(const AstNode &node);

  explicit Impl(ConsoleEngine *parent) noexcept : mParent(parent) {}
};

ConsoleEngine::ConsoleEngine()
    : ScriptEngineBase(), mImpl(std::make_unique<ConsoleEngine::Impl>(this)) {}

ConsoleEngine::~ConsoleEngine() = default;
ConsoleEngine::ConsoleEngine(ConsoleEngine &&) noexcept = default;
ConsoleEngine &ConsoleEngine::operator=(ConsoleEngine &&) noexcept = default;

std::unique_ptr<llvm::Module>
ConsoleEngine::Impl::compileStatement(const AstNode &node) {
  if (!node.is_root() || node.children.empty()) {
    // TODO: Cannot compile a partial AST, throw
    return nullptr;
  }

  auto module{mParent->makeModule("__console_statement")};
  mParent->addExternalFunsToModule(module.get());

  // The AST contains a single statement, which needs to be wrapped in an
  // anonymous function.
  auto &ctx{mParent->getContext()};
  llvm::IRBuilder irBuilder(ctx);

  auto *funType{llvm::FunctionType::get(llvm::Type::getVoidTy(ctx), {})};
  auto *fun{llvm::Function::Create(funType,
                                   llvm::Function::ExternalLinkage,
                                   "__console_statement_fun",
                                   module.get())};
  auto *bb{llvm::BasicBlock::Create(ctx, "entry", fun)};
  irBuilder.SetInsertPoint(bb);

  auto visitor{mParent->makeVisitor(module.get(), irBuilder)};
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

  auto module{mImpl->compileStatement(*root)};
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
      oo::scriptingLogger()->warn("JIT error: {}", e.message());
    });
    throw std::runtime_error("Failed to find statement address");
  }

  using func_t = void (*)();
  std::uintptr_t addr{*addrOrErr};
  auto func{reinterpret_cast<func_t>(addr)};

  func();
}

} // namespace oo