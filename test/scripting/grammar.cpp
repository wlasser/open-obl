#include "scripting/grammar.hpp"
#include <catch2/catch.hpp>
#include <tao/pegtl/analyze.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>
#include <iostream>

namespace pegtl = tao::TAO_PEGTL_NAMESPACE;

[[nodiscard]] std::string
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

void printAst(const pegtl::parse_tree::node &node) {
  scripting::visitAst(node, std::string{}, printVisitor);
}

template<class T> [[nodiscard]] auto parseScript(T &&in) {
  return pegtl::parse_tree::parse<scripting::Grammar,
                                  scripting::AstSelector>(in);
}

TEST_CASE("grammar is valid", "[scripting]") {
  const auto numIssues{pegtl::analyze<scripting::Grammar>()};
  REQUIRE(numIssues == 0);
}

TEST_CASE("can parse scriptname", "[scripting]") {
  const auto numIssues{pegtl::analyze<scripting::Grammar>()};
  REQUIRE(numIssues == 0);

  std::unique_ptr<pegtl::parse_tree::node> root{};

  SECTION("parse minimal scriptname") {
    const char *script = "scriptname MyScript";
    pegtl::memory_input in(script, "");
    root = parseScript(in);
  }

  SECTION("parse scriptname with surrounding comments") {
    const char *script = R"script(
; First comment
 scn MyScript ; This is the script name
    )script";
    pegtl::memory_input in(script, "");
    root = parseScript(in);
  }

  REQUIRE(root != nullptr);
  REQUIRE(!root->children.empty());

  const auto &scriptnameStatement{root->children[0]};
  REQUIRE(scriptnameStatement->children.size() > 1);

  const auto &keyword{scriptnameStatement->children[0]};
  REQUIRE(keyword->id == &typeid(scripting::RawScriptname));
  const auto &scriptname{scriptnameStatement->children[1]};
  REQUIRE(scriptname->has_content());
  REQUIRE(scriptname->content() == "MyScript");
}

TEST_CASE("fails to parse invalid scriptname") {
  {
    const char *script = "scriptname ;This is a comment";
    pegtl::memory_input in(script, "");
    REQUIRE_THROWS_AS(parseScript(in), pegtl::parse_error);
  }

  {
    const char *script = "scriptname 12hello";
    pegtl::memory_input in(script, "");
    REQUIRE_THROWS_AS(parseScript(in), pegtl::parse_error);
  }

  {
    const char *script = "scnmore MyScript";
    pegtl::memory_input in(script, "");
    REQUIRE_THROWS_AS(parseScript(in), pegtl::parse_error);
  }

  {
    const char *script = "";
    pegtl::memory_input in(script, "");
    REQUIRE_THROWS_AS(parseScript(in), pegtl::parse_error);
  }
}
