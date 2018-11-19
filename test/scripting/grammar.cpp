#include <record/formid.hpp>
#include "scripting/grammar.hpp"
#include <catch2/catch.hpp>
#include <tao/pegtl/analyze.hpp>
#include <limits>
#include <string>
#include <string_view>

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

TEST_CASE("can parse block statements with integer modes", "[scripting]") {
  const auto numIssues{pegtl::analyze<scripting::Grammar>()};
  REQUIRE(numIssues == 0);

  const auto *script = R"script(
scn MyScript
begin MenuMode 4329 ; Some menu type
end
)script";
  pegtl::memory_input in(script, "");
  const auto root = parseScript(in);
  REQUIRE(root != nullptr);
  const auto &beginStatement{root->children[1]};
  REQUIRE(beginStatement->id == &typeid(scripting::BlockBeginStatement));
  REQUIRE(beginStatement->children.size() > 1);

  const auto &blockName{beginStatement->children[0]};
  REQUIRE(blockName->has_content());
  REQUIRE(blockName->content() == "MenuMode");

  const auto &blockType{beginStatement->children[1]};
  REQUIRE(blockType->id == &typeid(scripting::IntegerLiteral));
  REQUIRE(blockType->has_content());
  REQUIRE(std::stoi(blockType->content()) == 4329);

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

  {
    const char *script = R"script(
scn MyScript
begin GameMode
; No end statement!
    )script";
    pegtl::memory_input in(script, "");
    REQUIRE_THROWS_AS(parseScript(in), pegtl::parse_error);
  }
}

TEST_CASE("can parse string literals", "[scripting]") {
  auto parseLiteral = [](auto &&in) {
    return pegtl::parse_tree::parse<scripting::StringLiteral,
                                    scripting::AstSelector>(in);
  };

  auto requireHasString = [](const pegtl::parse_tree::node &root,
                             const std::string &expected) {
    REQUIRE_FALSE(root.children.empty());
    const auto &content{root.children[0]};
    REQUIRE(content->id == &typeid(scripting::StringLiteralContents));
    REQUIRE(content->has_content());
    REQUIRE(content->content() == expected);
  };

  {
    const auto *script = R"("Hello")";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    requireHasString(*root, "Hello");
  }

  {
    const auto *script = R"("")";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    requireHasString(*root, "");
  }

  {
    const auto *script = R"("This \t is not escaped")";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    requireHasString(*root, R"(This \t is not escaped)");
  }

  {
    const auto *script = R"("This is not
        a string)";
    pegtl::memory_input in(script, "");
    REQUIRE(parseLiteral(in) == nullptr);
  }

  {
    const auto *script = R"("This string" "Is two strings")";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    requireHasString(*root, R"(This string)");
  }

  {
    const auto *script = R"("This is " not a string)";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    requireHasString(*root, R"(This is )");
  }
}

TEST_CASE("can parse integer literals", "[scripting]") {
  auto parseLiteral = [](auto &&in) {
    return pegtl::parse_tree::parse<scripting::IntegerLiteral,
                                    scripting::AstSelector>(in);
  };

  auto requireHasInteger = [](const pegtl::parse_tree::node &root,
                              int expected) {
    REQUIRE_FALSE(root.children.empty());
    const auto &content{root.children[0]};
    REQUIRE(content->id == &typeid(scripting::IntegerLiteral));
    REQUIRE(content->has_content());
    REQUIRE(std::stoi(content->content()) == expected);
  };

  {
    const auto *script = "153";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    requireHasInteger(*root, 153);
  }

  {
    const auto *script = "0";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    requireHasInteger(*root, 0);
  }

  {
    const auto script = std::to_string(std::numeric_limits<int>::max());
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    requireHasInteger(*root, std::numeric_limits<int>::max());
  }
}

TEST_CASE("can parse ref literals", "[scripting]") {
  auto parseLiteral = [](auto &&in) {
    return pegtl::parse_tree::parse<scripting::RefLiteral,
                                    scripting::AstSelector>(in);
  };

  auto requireHasReference = [](const pegtl::parse_tree::node &root,
                                FormId expected) {
    REQUIRE_FALSE(root.children.empty());
    const auto &content{root.children[0]};
    REQUIRE(content->id == &typeid(scripting::RefLiteralContents));
    REQUIRE(content->has_content());
    REQUIRE(std::stoi(content->content(), nullptr, 16) == expected);
  };

  {
    const auto *script = "#00103a5F";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    requireHasReference(*root, 0x00103a5f);
  }

  {
    const auto *script = "#";
    pegtl::memory_input in(script, "");
    REQUIRE(parseLiteral(in) == nullptr);
  }

  {
    const auto *script = "##509a";
    pegtl::memory_input in(script, "");
    REQUIRE(parseLiteral(in) == nullptr);
  }

  {
    const auto *script = "30915fab";
    pegtl::memory_input in(script, "");
    REQUIRE(parseLiteral(in) == nullptr);
  }

  {
    const auto *script = "#000";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    requireHasReference(*root, 0);
  }
}

TEST_CASE("can parse floating point literals", "[scripting]") {
  auto parseLiteral = [](auto &&in) {
    return pegtl::parse_tree::parse<scripting::FloatLiteral,
                                    scripting::AstSelector>(in);
  };

  auto requireHasFloat = [](const pegtl::parse_tree::node &root,
                            float expected) {
    REQUIRE_FALSE(root.children.empty());
    const auto &content{root.children[0]};
    REQUIRE(content->id == &typeid(scripting::FloatLiteral));
    REQUIRE(content->has_content());
    REQUIRE_THAT(std::stof(content->content()), Catch::WithinULP(expected, 1));
  };

  {
    const auto *script = "3.14159";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    requireHasFloat(*root, 3.14159f);
  }

  {
    const auto *script = "0.142";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    requireHasFloat(*root, 0.142f);
  }

  {
    const auto *script = "0.0001";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    requireHasFloat(*root, 0.0001f);
  }

  {
    const auto *script = ".142";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    requireHasFloat(*root, 0.142f);
  }

  {
    const auto *script = ".0001";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    requireHasFloat(*root, 0.0001f);
  }

  {
    const auto *script = "01.32";
    pegtl::memory_input in(script, "");
    REQUIRE(parseLiteral(in) == nullptr);
  }

  {
    const auto *script = ".";
    pegtl::memory_input in(script, "");
    REQUIRE(parseLiteral(in) == nullptr);
  }

  {
    const auto *script = "3.1.4";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    requireHasFloat(*root, 3.1f);
  }
}

TEST_CASE("can parse literals", "[scripting]") {
  auto parseLiteral = [](auto &&in) {
    return pegtl::parse_tree::parse<scripting::Literal,
                                    scripting::AstSelector>(in);
  };

  {
    const auto *script = "3.14";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    REQUIRE(root->children[0]->id == &typeid(scripting::FloatLiteral));
  }

  {
    const auto *script = "359";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    REQUIRE(root->children[0]->id == &typeid(scripting::IntegerLiteral));
  }

  {
    const auto *script = "0";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    REQUIRE(root->children[0]->id == &typeid(scripting::IntegerLiteral));
  }

  {
    const auto *script = R"("359")";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    REQUIRE(root->children[0]->id == &typeid(scripting::StringLiteralContents));
  }
  {
    const auto *script = "#59";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    REQUIRE(root->children[0]->id == &typeid(scripting::RefLiteralContents));
  }
}

TEST_CASE("can declare variables", "[scripting]") {
  const auto numIssues{pegtl::analyze<scripting::Grammar>()};
  REQUIRE(numIssues == 0);

  auto requireVariable = [](const pegtl::parse_tree::node &node,
                            auto &&type, const std::string &name) {
    REQUIRE(node.children.size() == 2);
    REQUIRE(node.children[0]->id == type);
    REQUIRE(node.children[1]->has_content());
    REQUIRE(node.children[1]->content() == name);
  };

  // All variable declarations follow the same pattern, so there is minimal
  // advantage to testing them all separately.

  const char *script = R"script(
scn MyScript
begin GameMode
    short MyVar ; This is a short
      short    my2Var39
    ref myRef

    ; It's pretty long
    long long ; looong
    short long;Yes, you can use keywords as identifiers

    float
       f
end
    )script";
  pegtl::memory_input in(script, "");
  const auto root = parseScript(in);
  REQUIRE(root != nullptr);
  REQUIRE(root->children.size() > 8);

  const auto &decl1{root->children[2]};
  requireVariable(*decl1, &typeid(scripting::RawShort), "MyVar");

  const auto &decl2{root->children[3]};
  requireVariable(*decl2, &typeid(scripting::RawShort), "my2Var39");

  const auto &decl3{root->children[4]};
  requireVariable(*decl3, &typeid(scripting::RawRef), "myRef");

  const auto &decl4{root->children[5]};
  requireVariable(*decl4, &typeid(scripting::RawLong), "long");

  const auto &decl5{root->children[6]};
  requireVariable(*decl5, &typeid(scripting::RawShort), "long");

  const auto &decl6{root->children[7]};
  requireVariable(*decl6, &typeid(scripting::RawFloat), "f");
}