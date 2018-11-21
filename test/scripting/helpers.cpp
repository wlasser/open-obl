#include "helpers.hpp"
#include <catch2/catch.hpp>
#include <iostream>
#include <string>
#include <string_view>

namespace scripting {

[[nodiscard]] bool isStatement(const pegtl::parse_tree::node &node) {
  const std::string_view name{node.id->name()};
  return name.find("Statement") != std::string_view::npos;
}

[[nodiscard]] std::string
printVisitor(const pegtl::parse_tree::node &node, const std::string &indent) {
  if (node.is_root()) {
    std::cout << "ROOT\n";
  } else {
    std::cout << indent << node.name();
    if (node.has_content() && !isStatement(node)) {
      std::cout << " \"" << node.content() << '"';
    }
    std::cout << " at " << node.begin() << " to " << node.end() << '\n';
  }
  return indent + "| ";
};

void printAst(const pegtl::parse_tree::node &node) {
  scripting::visitAst(node, std::string{}, printVisitor);
}

[[nodiscard]] bool
isString(const pegtl::parse_tree::node &node, std::string_view expected) {
  return node.is<scripting::StringLiteralContents>()
      && node.has_content()
      && node.content() == expected;
}

[[nodiscard]] bool
isInteger(const pegtl::parse_tree::node &node, int expected) {
  return node.is<scripting::IntegerLiteral>()
      && node.has_content()
      && std::stoi(node.content()) == expected;
}

[[nodiscard]] bool
isReference(const pegtl::parse_tree::node &node, FormId expected) {
  return node.is<scripting::RefLiteralContents>()
      && node.has_content()
      && std::stoi(node.content(), nullptr, 16) == expected;
}

[[nodiscard]] bool
isFloat(const pegtl::parse_tree::node &node, float expected) {
  return node.id == &typeid(scripting::FloatLiteral)
      && node.has_content()
      && Catch::WithinULP(expected, 1).match(std::stof(node.content()));
}

} // namespace scripting
