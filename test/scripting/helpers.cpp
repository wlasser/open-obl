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

void requireHasString(const pegtl::parse_tree::node &root,
                      const std::string &expected) {
  REQUIRE_FALSE(root.children.empty());
  const auto &content{root.children[0]};
  REQUIRE(content->id == &typeid(scripting::StringLiteralContents));
  REQUIRE(content->has_content());
  REQUIRE(content->content() == expected);
}

void requireHasInteger(const pegtl::parse_tree::node &root, int expected) {
  REQUIRE_FALSE(root.children.empty());
  const auto &content{root.children[0]};
  REQUIRE(content->id == &typeid(scripting::IntegerLiteral));
  REQUIRE(content->has_content());
  REQUIRE(std::stoi(content->content()) == expected);
}

void requireHasReference(const pegtl::parse_tree::node &root, FormId expected) {
  REQUIRE_FALSE(root.children.empty());
  const auto &content{root.children[0]};
  REQUIRE(content->id == &typeid(scripting::RefLiteralContents));
  REQUIRE(content->has_content());
  REQUIRE(std::stoi(content->content(), nullptr, 16) == expected);
}

void requireHasFloat(const pegtl::parse_tree::node &root, float expected) {
  REQUIRE_FALSE(root.children.empty());
  const auto &content{root.children[0]};
  REQUIRE(content->id == &typeid(scripting::FloatLiteral));
  REQUIRE(content->has_content());
  REQUIRE_THAT(std::stof(content->content()), Catch::WithinULP(expected, 1));
}

} // namespace scripting
