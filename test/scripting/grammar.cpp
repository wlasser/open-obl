#include "helpers.hpp"
#include "record/formid.hpp"
#include "scripting/grammar.hpp"
#include <catch2/catch.hpp>
#include <tao/pegtl/analyze.hpp>
#include <limits>
#include <memory>
#include <string>

namespace pegtl = tao::TAO_PEGTL_NAMESPACE;

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
    root = scripting::parseScript(in);
  }

  SECTION("parse scriptname with surrounding comments") {
    const char *script = R"script(
; First comment
 scn MyScript ; This is the script name
   ; Script body goes here!
    )script";
    pegtl::memory_input in(script, "");
    root = scripting::parseScript(in);
  }

  REQUIRE(root != nullptr);
  REQUIRE_FALSE(root->children.empty());

  const auto &scriptnameStatement{root->children[0]};
  REQUIRE(scriptnameStatement->children.size() > 1);

  const auto &keyword{scriptnameStatement->children[0]};
  REQUIRE(keyword->is<scripting::RawScriptname>());
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
    REQUIRE_THROWS_AS(scripting::parseScript(in), pegtl::parse_error);
  }

  {
    const char *script = "scriptname 12hello";
    pegtl::memory_input in(script, "");
    REQUIRE_THROWS_AS(scripting::parseScript(in), pegtl::parse_error);
  }

  {
    const char *script = "scnmore MyScript";
    pegtl::memory_input in(script, "");
    REQUIRE_THROWS_AS(scripting::parseScript(in), pegtl::parse_error);
  }

  {
    const char *script = "";
    pegtl::memory_input in(script, "");
    REQUIRE_THROWS_AS(scripting::parseScript(in), pegtl::parse_error);
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
    root = scripting::parseScript(in);
  }

  SECTION("parse block statement with surrounding comments") {
    const char *script = R"script(
scn MyScript
begin   GameMode ; Runs during gameplay
; This is where I'd put my script, if I had one
     end ; End of script
    )script";
    pegtl::memory_input in(script, "");
    root = scripting::parseScript(in);
  }

  SECTION("parse block statement with unexpected name") {
    const char *script = R"script(
scn MyScript
begin begin
end
)script";
    pegtl::memory_input in(script, "");
    root = scripting::parseScript(in);
    expectedBlockname = "begin";
  }

  REQUIRE(root != nullptr);
  REQUIRE(root->children.size() > 2);

  const auto &beginStatement{root->children[1]};
  REQUIRE(beginStatement->is<scripting::BlockBeginStatement>());
  REQUIRE_FALSE(beginStatement->children.empty());

  const auto &blockName{beginStatement->children[0]};
  REQUIRE(blockName->has_content());
  REQUIRE(blockName->content() == expectedBlockname);

  const auto &endStatement{root->children[2]};
  REQUIRE(endStatement->is<scripting::BlockEndStatement>());
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
  const auto root = scripting::parseScript(in);
  REQUIRE(root != nullptr);
  const auto &beginStatement{root->children[1]};
  REQUIRE(beginStatement->is<scripting::BlockBeginStatement>());
  REQUIRE(beginStatement->children.size() > 1);

  const auto &blockName{beginStatement->children[0]};
  REQUIRE(blockName->has_content());
  REQUIRE(blockName->content() == "MenuMode");

  const auto &blockType{beginStatement->children[1]};
  REQUIRE(blockType->is<scripting::IntegerLiteral>());
  REQUIRE(blockType->has_content());
  REQUIRE(std::stoi(blockType->content()) == 4329);

  const auto &endStatement{root->children[2]};
  REQUIRE(endStatement->is<scripting::BlockEndStatement>());
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
    root = scripting::parseScript(in);
  }

  SECTION("parse poorly formatted blocks") {
    const auto *script = R"script(
scn MyScript begin GameMode end begin MenuMode
end
)script";
    pegtl::memory_input in(script, "");
    root = scripting::parseScript(in);
  }

  REQUIRE(root != nullptr);
  REQUIRE(root->children.size() > 4);

  {
    const auto &beginStatement{root->children[1]};
    REQUIRE(beginStatement->is<scripting::BlockBeginStatement>());
    REQUIRE_FALSE(beginStatement->children.empty());

    const auto &blockName{beginStatement->children[0]};
    REQUIRE(blockName->has_content());
    REQUIRE(blockName->content() == "GameMode");

    const auto &endStatement{root->children[2]};
    REQUIRE(endStatement->is<scripting::BlockEndStatement>());
  }

  {
    const auto &beginStatement{root->children[3]};
    REQUIRE(beginStatement->is<scripting::BlockBeginStatement>());
    REQUIRE_FALSE(beginStatement->children.empty());

    const auto &blockName{beginStatement->children[0]};
    REQUIRE(blockName->has_content());
    REQUIRE(blockName->content() == "MenuMode");

    const auto &endStatement{root->children[4]};
    REQUIRE(endStatement->is<scripting::BlockEndStatement>());
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
    REQUIRE_THROWS_AS(scripting::parseScript(in), pegtl::parse_error);
  }

  {
    const char *script = R"script(
scn MyScript
begin begin begin
end
    )script";
    pegtl::memory_input in(script, "");
    REQUIRE_THROWS_AS(scripting::parseScript(in), pegtl::parse_error);
  }

  {
    const char *script = R"script(
scn MyScript
begin GameMode
; No end statement!
    )script";
    pegtl::memory_input in(script, "");
    REQUIRE_THROWS_AS(scripting::parseScript(in), pegtl::parse_error);
  }
}

TEST_CASE("can parse string literals", "[scripting]") {
  auto parseLiteral = [](auto &&in) {
    return pegtl::parse_tree::parse<scripting::StringLiteral,
                                    scripting::AstSelector>(in);
  };

  {
    const auto *script = R"("Hello")";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    scripting::requireIsString(*root->children[0], "Hello");
  }

  {
    const auto *script = R"("")";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    scripting::requireIsString(*root->children[0], "");
  }

  {
    const auto *script = R"("This \t is not escaped")";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    scripting::requireIsString(*root->children[0], R"(This \t is not escaped)");
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
    REQUIRE_FALSE(root->children.empty());
    scripting::requireIsString(*root->children[0], R"(This string)");
  }

  {
    const auto *script = R"("This is " not a string)";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    scripting::requireIsString(*root->children[0], R"(This is )");
  }
}

TEST_CASE("can parse integer literals", "[scripting]") {
  auto parseLiteral = [](auto &&in) {
    return pegtl::parse_tree::parse<scripting::IntegerLiteral,
                                    scripting::AstSelector>(in);
  };

  {
    const auto *script = "153";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    scripting::requireIsInteger(*root->children[0], 153);
  }

  {
    const auto *script = "0";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    scripting::requireIsInteger(*root->children[0], 0);
  }

  {
    const auto script = std::to_string(std::numeric_limits<int>::max());
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    scripting::requireIsInteger(*root->children[0],
                                std::numeric_limits<int>::max());
  }
}

TEST_CASE("can parse ref literals", "[scripting]") {
  auto parseLiteral = [](auto &&in) {
    return pegtl::parse_tree::parse<scripting::RefLiteral,
                                    scripting::AstSelector>(in);
  };

  {
    const auto *script = "#00103a5F";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    scripting::requireIsReference(*root->children[0], 0x00103a5f);
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
    REQUIRE_FALSE(root->children.empty());
    scripting::requireIsReference(*root->children[0], 0);
  }
}

TEST_CASE("can parse floating point literals", "[scripting]") {
  auto parseLiteral = [](auto &&in) {
    return pegtl::parse_tree::parse<scripting::FloatLiteral,
                                    scripting::AstSelector>(in);
  };

  {
    const auto *script = "3.14159";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    scripting::requireIsFloat(*root->children[0], 3.14159f);
  }

  {
    const auto *script = "0.142";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    scripting::requireIsFloat(*root->children[0], 0.142f);
  }

  {
    const auto *script = "0.0001";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    scripting::requireIsFloat(*root->children[0], 0.0001f);
  }

  {
    const auto *script = ".142";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    scripting::requireIsFloat(*root->children[0], 0.142f);
  }

  {
    const auto *script = ".0001";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    scripting::requireIsFloat(*root->children[0], 0.0001f);
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
    REQUIRE_FALSE(root->children.empty());
    scripting::requireIsFloat(*root->children[0], 3.1f);
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
    REQUIRE(root->children[0]->is<scripting::FloatLiteral>());
  }

  {
    const auto *script = "359";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    REQUIRE(root->children[0]->is<scripting::IntegerLiteral>());
  }

  {
    const auto *script = "0";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    REQUIRE(root->children[0]->is<scripting::IntegerLiteral>());
  }

  {
    const auto *script = R"("359")";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    REQUIRE(root->children[0]->is<scripting::StringLiteralContents>());
  }
  {
    const auto *script = "#59";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    REQUIRE(root->children[0]->is<scripting::RefLiteralContents>());
  }
}

TEST_CASE("can parse expressions", "[scripting]") {
  auto parseExpression = [](auto &&in) {
    return pegtl::parse_tree::parse<scripting::Expression,
                                    scripting::AstSelector>(in);
  };

  {
    const auto *script = "3.75";
    pegtl::memory_input in(script, "");
    const auto root = parseExpression(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    REQUIRE(root->children[0]->is<scripting::FloatLiteral>());
  }

  {
    const auto *script = "-75";
    pegtl::memory_input in(script, "");
    const auto root = parseExpression(in);
    REQUIRE(root != nullptr);
    REQUIRE(!root->children.empty());

    const auto &op{root->children[0]};
    REQUIRE(op->is<scripting::StrDash>());
    REQUIRE_FALSE(op->children.empty());

    const auto &literal{op->children[0]};
    REQUIRE(literal->is<scripting::IntegerLiteral>());
  }

  {
    const auto *script = "+75";
    pegtl::memory_input in(script, "");
    const auto root = parseExpression(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());

    const auto &op{root->children[0]};
    REQUIRE(op->is<scripting::StrPlus>());
    REQUIRE_FALSE(op->children.empty());

    const auto &literal{op->children[0]};
    REQUIRE(literal->is<scripting::IntegerLiteral>());
  }

  {
    const auto *script = "(75)";
    pegtl::memory_input in(script, "");
    const auto root = parseExpression(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());

    const auto &literal{root->children[0]};
    REQUIRE(literal->is<scripting::IntegerLiteral>());
  }

  {
    const auto *script = "(  ( (((75) ))  ))";
    pegtl::memory_input in(script, "");
    const auto root = parseExpression(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());

    const auto &literal{root->children[0]};
    REQUIRE(literal->is<scripting::IntegerLiteral>());
  }

  {
    const auto *script = "39 || -75";
    pegtl::memory_input in(script, "");
    const auto root = parseExpression(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());

    const auto &op{root->children[0]};
    REQUIRE(op->is<scripting::StrOr>());
    REQUIRE(op->children.size() == 2);

    const auto &literal{op->children[0]};
    REQUIRE(literal->is<scripting::IntegerLiteral>());

    const auto &subExpr{op->children[1]};
    REQUIRE(subExpr->is<scripting::StrDash>());
  }

  {
    const auto *script = "1 && 2 || 3 && 4";
    pegtl::memory_input in(script, "");
    const auto root = parseExpression(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());

    const auto &orOp{root->children[0]};
    REQUIRE(orOp->is<scripting::StrOr>());
    REQUIRE(orOp->children.size() == 2);

    const auto &lhs{orOp->children[0]};
    REQUIRE(lhs->is<scripting::StrAnd>());

    const auto &rhs{orOp->children[1]};
    REQUIRE(rhs->is<scripting::StrAnd>());
  }

  {
    const auto *script = "(1 || 2) && (3 || 4)";
    pegtl::memory_input in(script, "");
    const auto root = parseExpression(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());

    const auto &andOp{root->children[0]};
    REQUIRE(andOp->is<scripting::StrAnd>());
    REQUIRE(andOp->children.size() == 2);

    const auto &lhs{andOp->children[0]};
    REQUIRE(lhs->is<scripting::StrOr>());

    const auto &rhs{andOp->children[1]};
    REQUIRE(rhs->is<scripting::StrOr>());
  }

  {
    const auto *script = "2 * 3 / 4 * 2";
    pegtl::memory_input in(script, "");
    const auto root = parseExpression(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());

    const auto &mulOp{root->children[0]};
    REQUIRE(mulOp->is<scripting::StrStar>());
    REQUIRE(mulOp->children.size() == 2);

    const auto &divOp{mulOp->children[0]};
    REQUIRE(divOp->is<scripting::StrSlash>());
    REQUIRE(divOp->children.size() == 2);

    const auto &mulOp2{divOp->children[0]};
    REQUIRE(mulOp2->is<scripting::StrStar>());
    REQUIRE(mulOp2->children.size() == 2);

    scripting::requireIsInteger(*mulOp2->children[0], 2);
    scripting::requireIsInteger(*mulOp2->children[1], 3);
    scripting::requireIsInteger(*divOp->children[1], 4);
    scripting::requireIsInteger(*mulOp->children[1], 2);
  }

  {
    const auto *script = "1 + 2 * 3";
    pegtl::memory_input in(script, "");
    const auto root = parseExpression(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());

    const auto &plusOp{root->children[0]};
    REQUIRE(plusOp->is<scripting::StrPlus>());
    REQUIRE(plusOp->children.size() == 2);

    scripting::requireIsInteger(*plusOp->children[0], 1);

    const auto &mulOp{plusOp->children[1]};
    REQUIRE(mulOp->is<scripting::StrStar>());
    REQUIRE(mulOp->children.size() == 2);

    scripting::requireIsInteger(*mulOp->children[0], 2);
    scripting::requireIsInteger(*mulOp->children[1], 3);
  }

  {
    const auto *script = "1 && 2 * 3 <= 4";
    pegtl::memory_input in(script, "");
    const auto root = parseExpression(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());

    const auto &andOp{root->children[0]};
    REQUIRE(andOp->is<scripting::StrAnd>());
    REQUIRE(andOp->children.size() == 2);

    scripting::requireIsInteger(*andOp->children[0], 1);

    const auto &lteqOp{andOp->children[1]};
    REQUIRE(lteqOp->is<scripting::StrLteq>());
    REQUIRE(lteqOp->children.size() == 2);

    const auto &mulOp{lteqOp->children[0]};
    REQUIRE(mulOp->is<scripting::StrStar>());
    REQUIRE(mulOp->children.size() == 2);

    scripting::requireIsInteger(*mulOp->children[0], 2);
    scripting::requireIsInteger(*mulOp->children[1], 3);
    scripting::requireIsInteger(*lteqOp->children[1], 4);
  }

  {
    const auto *script = "3 != 4";
    pegtl::memory_input in(script, "");
    const auto root = parseExpression(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());

    const auto &neqOp{root->children[0]};
    REQUIRE(neqOp->is<scripting::StrNeq>());
    REQUIRE(neqOp->children.size() == 2);

    scripting::requireIsInteger(*neqOp->children[0], 3);
    scripting::requireIsInteger(*neqOp->children[1], 4);
  }
}

TEST_CASE("can declare and assign to variables", "[scripting]") {
  const auto numIssues{pegtl::analyze<scripting::Grammar>()};
  REQUIRE(numIssues == 0);

  // All variable declarations follow the same pattern, so there is minimal
  // advantage to testing them all separately.

  {
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
    const auto root{scripting::parseScript(in)};
    REQUIRE(root != nullptr);
    REQUIRE(root->children.size() > 8);

    scripting::requireHasVariable<scripting::RawShort>(*root->children[2],
                                                       "MyVar");
    scripting::requireHasVariable<scripting::RawShort>(*root->children[3],
                                                       "my2Var39");
    scripting::requireHasVariable<scripting::RawRef>(*root->children[4],
                                                     "myRef");
    scripting::requireHasVariable<scripting::RawLong>(*root->children[5],
                                                      "long");
    scripting::requireHasVariable<scripting::RawShort>(*root->children[6],
                                                       "long");
    scripting::requireHasVariable<scripting::RawFloat>(*root->children[7],
                                                       "f");
  }

  {
    const char *script = R"script(
scn MyScript
short myGlobal
begin GameMode
end float myOtherGlobal
begin MenuMode long myLocal
end
short uselessVariable
    )script";

    pegtl::memory_input in(script, "");
    const auto root{scripting::parseScript(in)};
    REQUIRE(root != nullptr);
    REQUIRE(root->children.size() == 9);

    scripting::requireHasVariable<scripting::RawShort>(*root->children[1],
                                                       "myGlobal");
    scripting::requireHasVariable<scripting::RawFloat>(*root->children[4],
                                                       "myOtherGlobal");
    scripting::requireHasVariable<scripting::RawLong>(*root->children[6],
                                                      "myLocal");
    scripting::requireHasVariable<scripting::RawShort>(*root->children[8],
                                                       "uselessVariable");
  }

  {
    const char *script = R"script(
scn MyScript
begin GameMode
float short
short float
set float  to    3 ; Totally not confusing
set short to3.5 ; Isn't this language great?
end
    )script";

    pegtl::memory_input in(script, "");
    const auto root{scripting::parseScript(in)};
    REQUIRE(root != nullptr);
    REQUIRE(root->children.size() == 7);

    scripting::requireHasVariable<scripting::RawFloat>(*root->children[2],
                                                       "short");
    scripting::requireHasVariable<scripting::RawShort>(*root->children[3],
                                                       "float");
    const auto &set1{root->children[4]};
    REQUIRE(set1->is<scripting::SetStatement>());
    REQUIRE(set1->children.size() == 2);

    const auto &set1Name{set1->children[0]};
    REQUIRE(set1Name->has_content());
    REQUIRE(set1Name->content() == "float");
    scripting::requireIsInteger(*set1->children[1], 3);

    const auto &set2{root->children[5]};
    REQUIRE(set2->is<scripting::SetStatement>());
    REQUIRE(set2->children.size() == 2);

    const auto &set2Name{set2->children[0]};
    REQUIRE(set2Name->has_content());
    REQUIRE(set2Name->content() == "short");
    scripting::requireIsFloat(*set2->children[1], 3.5f);
  }

  {
    const char *script = R"script(
scn MyScript
begin GameMode
set SomeQuest.foo to SomeQuest.foo * 2
set #001234ab.bar to 8
end
    )script";

    pegtl::memory_input in(script, "");
    const auto root{scripting::parseScript(in)};
    REQUIRE(root != nullptr);
    REQUIRE(root->children.size() == 5);

    const auto &set1{root->children[2]};
    REQUIRE(set1->is<scripting::SetStatement>());
    REQUIRE(set1->children.size() == 2);

    const auto &set1Dest{set1->children[0]};
    REQUIRE(set1Dest->is<scripting::MemberAccess>());
    REQUIRE(set1Dest->children.size() == 2);

    REQUIRE(set1Dest->children[0]->is<scripting::RawIdentifier>());
    REQUIRE(set1Dest->children[0]->has_content());
    REQUIRE(set1Dest->children[0]->content() == "SomeQuest");

    REQUIRE(set1Dest->children[1]->is<scripting::RawIdentifier>());
    REQUIRE(set1Dest->children[1]->has_content());
    REQUIRE(set1Dest->children[1]->content() == "foo");

    const auto &set1Src{set1->children[1]};
    REQUIRE(set1Src->is<scripting::StrStar>());
    REQUIRE(set1Src->children.size() == 2);
    scripting::requireIsInteger(*set1Src->children[1], 2);

    const auto &set1SrcVar{set1Src->children[0]};
    REQUIRE(set1SrcVar->is<scripting::MemberAccess>());
    REQUIRE(set1SrcVar->children.size() == 2);

    REQUIRE(set1SrcVar->children[0]->is<scripting::RawIdentifier>());
    REQUIRE(set1SrcVar->children[0]->has_content());
    REQUIRE(set1SrcVar->children[0]->content() == "SomeQuest");

    REQUIRE(set1SrcVar->children[1]->is<scripting::RawIdentifier>());
    REQUIRE(set1SrcVar->children[1]->has_content());
    REQUIRE(set1SrcVar->children[1]->content() == "foo");

    const auto &set2{root->children[3]};
    REQUIRE(set2->is<scripting::SetStatement>());
    REQUIRE(set2->children.size() == 2);

    const auto &set2Dest{set2->children[0]};
    REQUIRE(set2Dest->is<scripting::MemberAccess>());
    REQUIRE(set2Dest->children.size() == 2);

    REQUIRE(set2Dest->children[0]->is<scripting::RefLiteralContents>());
    REQUIRE(set2Dest->children[0]->has_content());
    REQUIRE(set2Dest->children[0]->content() == "001234ab");

    REQUIRE(set2Dest->children[1]->is<scripting::RawIdentifier>());
    REQUIRE(set2Dest->children[1]->has_content());
    REQUIRE(set2Dest->children[1]->content() == "bar");

    scripting::requireIsInteger(*set2->children[1], 8);
  }
}
