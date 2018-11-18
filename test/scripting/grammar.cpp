#include "scripting/grammar.hpp"
#include <catch2/catch.hpp>
#include <tao/pegtl/analyze.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>
#include <iostream>

namespace pegtl = tao::TAO_PEGTL_NAMESPACE;

TEST_CASE("grammar is valid", "[scripting]") {
  const auto numIssues{pegtl::analyze<scripting::Grammar>()};
  REQUIRE(numIssues == 0);
}

void print_node(std::ostream &os,
                const pegtl::parse_tree::node &node,
                const std::string &indent = "") {
  if (node.is_root()) {
    os << "ROOT\n";
  } else {
    if (node.has_content()) {
      os << indent
         << node.name()
         << " \"" << node.content() << "\" at "
         << node.begin() << " to " << node.end()
         << '\n';
    } else {
      os << indent
         << node.name()
         << " at "
         << node.begin() << " to " << node.end()
         << '\n';
    }
  }
  for (const auto &child : node.children) {
    print_node(os, *child, indent + "  ");
  }
}

std::string
printVisitor(const pegtl::parse_tree::node &node, const std::string &indent) {
  if (node.is_root()) {
    std::cout << "ROOT\n";
  } else {
    std::cout << indent << node.name();
    if (node.has_content()) std::cout << " \"" << node.content() << '"';
    std::cout << " at " << node.begin() << " to " << node.end() << '\n';
  }
  return indent + "| ";
};

TEST_CASE("can parse scriptname", "[scripting]") {
  const auto numIssues{pegtl::analyze<scripting::Grammar>()};
  REQUIRE(numIssues == 0);

  {
    const char *script = "scriptname MyScript";
    pegtl::memory_input in(script, "");
    const auto root{pegtl::parse_tree::parse<scripting::Grammar,
                                             scripting::AstSelector>(in)};

    REQUIRE(root != nullptr);
    REQUIRE(!root->children.empty());

    const auto &scriptnameStatement{root->children[0]};
    REQUIRE(!scriptnameStatement->children.empty());

    const auto &scriptname{scriptnameStatement->children[1]};
    REQUIRE(scriptname->has_content());
    REQUIRE(scriptname->content() == "MyScript");
  }

  {
    const char *script = R"script(; First comment
 scn MyScript ; This is the script name
    )script";
    pegtl::memory_input in(script, "");
    const auto root{pegtl::parse_tree::parse<scripting::Grammar,
                                             scripting::AstSelector>(in)};

    REQUIRE(root != nullptr);
    REQUIRE(!root->children.empty());

    const auto &scriptnameStatement{root->children[0]};
    REQUIRE(!scriptnameStatement->children.empty());

    const auto &scriptname{scriptnameStatement->children[1]};
    REQUIRE(scriptname->has_content());
    REQUIRE(scriptname->content() == "MyScript");

    //scripting::visitAst(*root, std::string{}, printVisitor);
  }
}