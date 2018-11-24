#include "helpers.hpp"
#include <catch2/catch.hpp>
#include <iostream>
#include <string>
#include <string_view>

namespace scripting {

[[nodiscard]] bool isStatement(const AstNode &node) {
  const std::string_view name{node.name()};
  return name.find("Statement") != std::string_view::npos;
}

void printNode(const AstNode &node) {
  constexpr std::string_view prefix{"scripting::"};
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
  scripting::printAstImpl(node, "");
}

[[nodiscard]] bool isString(const AstNode &node, std::string_view expected) {
  return node.is<scripting::StringLiteralContents>()
      && node.has_content()
      && node.content() == expected;
}

[[nodiscard]] bool isInteger(const AstNode &node, int expected) {
  return node.is<scripting::IntegerLiteral>()
      && node.has_content()
      && std::stoi(node.content()) == expected;
}

[[nodiscard]] bool isReference(const AstNode &node, FormId expected) {
  return node.is<scripting::RefLiteralContents>()
      && node.has_content()
      && std::stoi(node.content(), nullptr, 16) == expected;
}

[[nodiscard]] bool isFloat(const AstNode &node, float expected) {
  return node.is<scripting::FloatLiteral>()
      && node.has_content()
      && Catch::WithinULP(expected, 1).match(std::stof(node.content()));
}

} // namespace scripting
