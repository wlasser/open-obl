#include "helpers.hpp"
#include <catch2/catch.hpp>
#include <cstdio>
#include <iostream>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <string>
#include <string_view>

int Func(int x) {
  return 9 * x;
}

int ConsoleFunc(int x) {
  printf("%d\n", x);
  return 0;
}

int MemberFunc(uint32_t mem, int x) {
  return mem * x;
}

namespace oo {

[[nodiscard]] bool isStatement(const AstNode &node) {
  const std::string name{node.name()};
  return name.find("Statement") != std::string::npos;
}

void printNode(const AstNode &node) {
  constexpr std::string_view prefix{"oo::grammar::"};
  std::string name{node.name()};
  //C++20: if (name.begins_with(prefix)) {
  if (name.length() > prefix.length()) {
    if (std::string_view pre(name.data(), prefix.length()); pre == prefix) {
      name.erase(0, prefix.length());
    }
  }

  std::cout << name;
  if (node.has_content() && !isStatement(node)) {
    std::cout << " \"" << node.content() << '"';
  } else if (!node.getValue().empty()) {
    std::cout << " \"" << node.getValue() << '"';
  }

  const auto begin{node.begin()};
  const auto end{node.end()};
  std::cout << " (" << begin.line << ":" << begin.byte_in_line << ", "
            << end.line << ":" << end.byte_in_line << ")\n";
}

void printAstImpl(const AstNode &node, const std::string &indent) {
  if (node.is_root()) {
    std::cout << "ROOT\n";
  } else {
    printNode(node);
  }
  // Thanks to the Clang AST for the formatting inspiration
  for (auto it{node.children.begin()}; it != node.children.end(); ++it) {
    if (it == node.children.end() - 1) {
      std::cout << indent << "`-";
      printAstImpl(**it, indent + "  ");
    } else {
      std::cout << indent << "|-";
      printAstImpl(**it, indent + "| ");
    }
  }
}

void printAst(const AstNode &node) {
  oo::printAstImpl(node, "");
}

[[nodiscard]] bool isString(const AstNode &node, std::string_view expected) {
  return node.is<oo::grammar::StringLiteralContents>()
      && node.has_content()
      && node.content() == expected;
}

[[nodiscard]] bool isInteger(const AstNode &node, int expected) {
  return node.is<oo::grammar::IntegerLiteral>()
      && node.has_content()
      && std::stoi(node.content()) == expected;
}

[[nodiscard]] bool isReference(const AstNode &node, FormId expected) {
  return node.is<oo::grammar::RefLiteralContents>()
      && node.has_content()
      && std::stoi(node.content(), nullptr, 16) == expected;
}

[[nodiscard]] bool isFloat(const AstNode &node, float expected) {
  return node.is<oo::grammar::FloatLiteral>()
      && node.has_content()
      && Catch::WithinULP(expected, 1).match(std::stof(node.content()));
}

oo::ScriptEngine &getScriptEngine() {
  static oo::ScriptEngine eng = []() {
    oo::ScriptEngine eng{};
    eng.registerFunction<decltype(Func)>("Func");
    eng.registerFunction<decltype(MemberFunc)>("MemberFunc");
    return eng;
  }();

  static auto doOnce = []() {
    auto logger{spdlog::stderr_color_mt("scripting_test")};
    oo::scriptingLogger("scripting_test");
    return 0;
  };

  return eng;
}

} // namespace oo
