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

void requireIsString(const pegtl::parse_tree::node &node,
                     const std::string &expected) {
  REQUIRE(node.id == &typeid(scripting::StringLiteralContents));
  REQUIRE(node.has_content());
  REQUIRE(node.content() == expected);
}

void requireIsInteger(const pegtl::parse_tree::node &node, int expected) {
  REQUIRE(node.id == &typeid(scripting::IntegerLiteral));
  REQUIRE(node.has_content());
  REQUIRE(std::stoi(node.content()) == expected);
}

void requireIsReference(const pegtl::parse_tree::node &node, FormId expected) {
  REQUIRE(node.id == &typeid(scripting::RefLiteralContents));
  REQUIRE(node.has_content());
  REQUIRE(std::stoi(node.content(), nullptr, 16) == expected);
}

void requireIsFloat(const pegtl::parse_tree::node &node, float expected) {
  REQUIRE(node.id == &typeid(scripting::FloatLiteral));
  REQUIRE(node.has_content());
  REQUIRE_THAT(std::stof(node.content()), Catch::WithinULP(expected, 1));
}

} // namespace scripting
