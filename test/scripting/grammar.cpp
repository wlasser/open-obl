#include "scripting/grammar.hpp"
#include <catch2/catch.hpp>
#include <tao/pegtl/analyze.hpp>

namespace pegtl = tao::TAO_PEGTL_NAMESPACE;

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
   ; Script body goes here!
    )script";
    pegtl::memory_input in(script, "");
    root = parseScript(in);
  }

  REQUIRE(root != nullptr);
  REQUIRE_FALSE(root->children.empty());

  const auto &scriptnameStatement{root->children[0]};
  REQUIRE(scriptnameStatement->children.size() > 1);

  const auto &keyword{scriptnameStatement->children[0]};
  REQUIRE(keyword->id == &typeid(scripting::RawScriptname));
  const auto &scriptname{scriptnameStatement->children[1]};
  REQUIRE(scriptname->has_content());
  REQUIRE(scriptname->content() == "MyScript");
}

TEST_CASE("fails to parse invalid scriptname", "[scripting]") {
  const auto numIssues{pegtl::analyze<scripting::Grammar>()};
  REQUIRE(numIssues == 0);

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

TEST_CASE("can parse block statements", "[scripting]") {
  const auto numIssues{pegtl::analyze<scripting::Grammar>()};
  REQUIRE(numIssues == 0);

  std::unique_ptr<pegtl::parse_tree::node> root{};
  std::string expectedBlockname{"GameMode"};

  SECTION("parse minimal block statement") {
    const char *script = R"script(
scn MyScript

begin GameMode
end
)script";
    pegtl::memory_input in(script, "");
    root = parseScript(in);
  }

  SECTION("parse block statement with surrounding comments") {
    const char *script = R"script(
scn MyScript
begin   GameMode ; Runs during gameplay
; This is where I'd put my script, if I had one
     end ; End of script
    )script";
    pegtl::memory_input in(script, "");
    root = parseScript(in);
  }

  SECTION("parse block statement with unexpected name") {
    const char *script = R"script(
scn MyScript
begin begin
end
)script";
    pegtl::memory_input in(script, "");
    root = parseScript(in);
    expectedBlockname = "begin";
  }

  REQUIRE(root != nullptr);
  REQUIRE(root->children.size() > 2);

  const auto &beginStatement{root->children[1]};
  REQUIRE(beginStatement->id == &typeid(scripting::BlockBeginStatement));
  REQUIRE_FALSE(beginStatement->children.empty());

  const auto &blockName{beginStatement->children[0]};
  REQUIRE(blockName->has_content());
  REQUIRE(blockName->content() == expectedBlockname);

  const auto &endStatement{root->children[2]};
  REQUIRE(endStatement->id == &typeid(scripting::BlockEndStatement));
}

TEST_CASE("can parse multiple block statements", "[scripting]") {
  const auto numIssues{pegtl::analyze<scripting::Grammar>()};
  REQUIRE(numIssues == 0);

  std::unique_ptr<pegtl::parse_tree::node> root{};

  SECTION("parse nicely formatted blocks") {
    const auto *script = R"script(
scn MyScript

; First block
begin GameMode
end

; Next block
begin MenuMode
end
)script";
    pegtl::memory_input in(script, "");
    root = parseScript(in);
  }

  SECTION("parse poorly formatted blocks") {
    const auto *script = R"script(
scn MyScript begin GameMode end begin MenuMode
end
)script";
    pegtl::memory_input in(script, "");
    root = parseScript(in);
  }

  REQUIRE(root != nullptr);
  REQUIRE(root->children.size() > 4);

  {
    const auto &beginStatement{root->children[1]};
    REQUIRE(beginStatement->id == &typeid(scripting::BlockBeginStatement));
    REQUIRE_FALSE(beginStatement->children.empty());

    const auto &blockName{beginStatement->children[0]};
    REQUIRE(blockName->has_content());
    REQUIRE(blockName->content() == "GameMode");

    const auto &endStatement{root->children[2]};
    REQUIRE(endStatement->id == &typeid(scripting::BlockEndStatement));
  }

  {
    const auto &beginStatement{root->children[3]};
    REQUIRE(beginStatement->id == &typeid(scripting::BlockBeginStatement));
    REQUIRE_FALSE(beginStatement->children.empty());

    const auto &blockName{beginStatement->children[0]};
    REQUIRE(blockName->has_content());
    REQUIRE(blockName->content() == "MenuMode");

    const auto &endStatement{root->children[4]};
    REQUIRE(endStatement->id == &typeid(scripting::BlockEndStatement));
  }
}

TEST_CASE("fails to parse invalid block statements", "[scripting]") {
  const auto numIssues{pegtl::analyze<scripting::Grammar>()};
  REQUIRE(numIssues == 0);

  {
    const char *script = R"script(
scn MyScript
begin ; No block name
end
)script";
    pegtl::memory_input in(script, "");
    REQUIRE_THROWS_AS(parseScript(in), pegtl::parse_error);
  }

  {
    const char *script = R"script(
scn MyScript
begin begin begin
end
    )script";
    pegtl::memory_input in(script, "");
    REQUIRE_THROWS_AS(parseScript(in), pegtl::parse_error);
  }
}