#include "helpers.hpp"
#include "record/formid.hpp"
#include "scripting/ast.hpp"
#include "scripting/grammar.hpp"
#include <catch2/catch.hpp>
#include <tao/pegtl/analyze.hpp>
#include <limits>
#include <memory>
#include <string>

TEST_CASE("grammar is valid", "[scripting]") {
  const auto numIssues{pegtl::analyze<oo::grammar::Grammar>()};
  REQUIRE(numIssues == 0);
}

TEST_CASE("can parse scriptname", "[scripting]") {
  std::unique_ptr<oo::AstNode> root{};

  SECTION("parse minimal scriptname") {
    std::string_view script = "scriptname MyScript";
    pegtl::memory_input in(script, "");
    root = oo::parseScript(in);
  }

  SECTION("parse scriptname with surrounding comments") {
    std::string_view script = R"script(
; First comment
 scn MyScript ; This is the script name
   ; Script body goes here!
    )script";
    pegtl::memory_input in(script, "");
    root = oo::parseScript(in);
  }

  REQUIRE(root != nullptr);
  REQUIRE_FALSE(root->children.empty());

  const auto &scriptnameStatement{root->children[0]};
  REQUIRE(scriptnameStatement->children.size() > 1);

  const auto &keyword{scriptnameStatement->children[0]};
  REQUIRE(keyword->is<oo::grammar::RawScriptname>());
  const auto &scriptname{scriptnameStatement->children[1]};
  REQUIRE(scriptname->has_content());
  REQUIRE(scriptname->content() == "MyScript");
}

TEST_CASE("fails to parse invalid scriptname", "[scripting]") {
  {
    std::string_view script = "scriptname ;This is a comment";
    pegtl::memory_input in(script, "");
    REQUIRE_THROWS_AS(oo::parseScript(in), pegtl::parse_error);
  }

  {
    std::string_view script = "scriptname 12hello";
    pegtl::memory_input in(script, "");
    REQUIRE_THROWS_AS(oo::parseScript(in), pegtl::parse_error);
  }

  {
    std::string_view script = "scnmore MyScript";
    pegtl::memory_input in(script, "");
    REQUIRE_THROWS_AS(oo::parseScript(in), pegtl::parse_error);
  }

  {
    std::string_view script = "";
    pegtl::memory_input in(script, "");
    REQUIRE_THROWS_AS(oo::parseScript(in), pegtl::parse_error);
  }
}

TEST_CASE("can parse block statements", "[scripting]") {
  std::unique_ptr<oo::AstNode> root{};
  std::string expectedBlockname{"GameMode"};

  SECTION("parse minimal block statement") {
    std::string_view script = R"script(
scn MyScript

begin GameMode
end
)script";
    pegtl::memory_input in(script, "");
    root = oo::parseScript(in);
  }

  SECTION("parse block statement with surrounding comments") {
    std::string_view script = R"script(
scn MyScript
begin   GameMode ; Runs during gameplay
; This is where I'd put my script, if I had one
     end ; End of script
    )script";
    pegtl::memory_input in(script, "");
    root = oo::parseScript(in);
  }

  SECTION("parse block statement with unexpected name") {
    std::string_view script = R"script(
scn MyScript
begin begin
end
)script";
    pegtl::memory_input in(script, "");
    root = oo::parseScript(in);
    expectedBlockname = "begin";
  }

  REQUIRE(root != nullptr);
  REQUIRE(root->children.size() == 2);

  const auto &blockStatement{root->children[1]};
  REQUIRE(blockStatement->is<oo::grammar::BlockStatement>());
  REQUIRE_FALSE(blockStatement->children.empty());

  const auto &blockName{blockStatement->children[0]};
  REQUIRE(blockName->has_content());
  REQUIRE(blockName->content() == expectedBlockname);
}

TEST_CASE("can parse block statements with integer modes", "[scripting]") {
  const auto numIssues{pegtl::analyze<oo::grammar::Grammar>()};
  REQUIRE(numIssues == 0);

  std::string_view script = R"script(
scn MyScript
begin MenuMode 4329 ; Some menu type
end
)script";
  pegtl::memory_input in(script, "");
  const auto root = oo::parseScript(in);
  REQUIRE(root != nullptr);

  const auto &blockStatement{root->children[1]};
  REQUIRE(blockStatement->is<oo::grammar::BlockStatement>());
  REQUIRE(blockStatement->children.size() == 2);

  const auto &blockName{blockStatement->children[0]};
  REQUIRE(blockName->has_content());
  REQUIRE(blockName->content() == "MenuMode");

  const auto &blockType{blockStatement->children[1]};
  REQUIRE(blockType->is<oo::grammar::IntegerLiteral>());
  REQUIRE(blockType->has_content());
  REQUIRE(std::stoi(blockType->content()) == 4329);
}

TEST_CASE("can parse multiple block statements", "[scripting]") {
  std::unique_ptr<oo::AstNode> root{};

  SECTION("parse nicely formatted blocks") {
    std::string_view script = R"script(
scn MyScript

; First block
begin GameMode
end

; Next block
begin MenuMode
end
)script";
    pegtl::memory_input in(script, "");
    root = oo::parseScript(in);
  }

  SECTION("parse poorly formatted blocks") {
    std::string_view script = R"script(
scn MyScript begin GameMode end begin MenuMode
end
)script";
    pegtl::memory_input in(script, "");
    root = oo::parseScript(in);
  }

  REQUIRE(root != nullptr);
  REQUIRE(root->children.size() == 3);

  {
    const auto &blockStatement{root->children[1]};
    REQUIRE(blockStatement->is<oo::grammar::BlockStatement>());
    REQUIRE_FALSE(blockStatement->children.empty());

    const auto &blockName{blockStatement->children[0]};
    REQUIRE(blockName->has_content());
    REQUIRE(blockName->content() == "GameMode");
  }

  {
    const auto &blockStatement{root->children[2]};
    REQUIRE(blockStatement->is<oo::grammar::BlockStatement>());
    REQUIRE_FALSE(blockStatement->children.empty());

    const auto &blockName{blockStatement->children[0]};
    REQUIRE(blockName->has_content());
    REQUIRE(blockName->content() == "MenuMode");
  }
}

TEST_CASE("fails to parse invalid block statements", "[scripting]") {
  {
    std::string_view script = R"script(
scn MyScript
begin ; No block name
end
)script";
    pegtl::memory_input in(script, "");
    REQUIRE_THROWS_AS(oo::parseScript(in), pegtl::parse_error);
  }

  {
    std::string_view script = R"script(
scn MyScript
begin begin begin
end
    )script";
    pegtl::memory_input in(script, "");
    REQUIRE_THROWS_AS(oo::parseScript(in), pegtl::parse_error);
  }

  {
    std::string_view script = R"script(
scn MyScript
begin GameMode
; No end statement!
    )script";
    pegtl::memory_input in(script, "");
    REQUIRE_THROWS_AS(oo::parseScript(in), pegtl::parse_error);
  }
}

TEST_CASE("can parse string literals", "[scripting]") {
  auto parseLiteral = [](auto &&in) {
    return pegtl::parse_tree::parse<oo::grammar::StringLiteral,
                                    oo::AstNode,
                                    oo::AstSelector>(in);
  };

  {
    std::string_view script = R"("Hello")";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    REQUIRE(oo::isString(*root->children[0], "Hello"));
  }

  {
    std::string_view script = R"("")";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    REQUIRE(oo::isString(*root->children[0], ""));
  }

  {
    std::string_view script = R"("This \t is not escaped")";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    REQUIRE(oo::isString(*root->children[0],
                         R"(This \t is not escaped)"));
  }

  {
    std::string_view script = R"("This is not
        a string)";
    pegtl::memory_input in(script, "");
    REQUIRE(parseLiteral(in) == nullptr);
  }

  {
    std::string_view script = R"("This string" "Is two strings")";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    REQUIRE(oo::isString(*root->children[0], R"(This string)"));
  }

  {
    std::string_view script = R"("This is " not a string)";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    REQUIRE(oo::isString(*root->children[0], R"(This is )"));
  }
}

TEST_CASE("can parse integer literals", "[scripting]") {
  auto parseLiteral = [](auto &&in) {
    return pegtl::parse_tree::parse<oo::grammar::IntegerLiteral,
                                    oo::AstNode,
                                    oo::AstSelector>(in);
  };

  {
    std::string_view script = "153";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    REQUIRE(oo::isInteger(*root->children[0], 153));
  }

  {
    std::string_view script = "0";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    REQUIRE(oo::isInteger(*root->children[0], 0));
  }

  {
    const auto script = std::to_string(std::numeric_limits<int>::max());
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    REQUIRE(oo::isInteger(*root->children[0],
                          std::numeric_limits<int>::max()));
  }
}

TEST_CASE("can parse ref literals", "[scripting]") {
  auto parseLiteral = [](auto &&in) {
    return pegtl::parse_tree::parse<oo::grammar::RefLiteral,
                                    oo::AstNode,
                                    oo::AstSelector>(in);
  };

  {
    std::string_view script = "#00103a5F";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    REQUIRE(oo::isReference(*root->children[0], 0x00103a5f));
  }

  {
    std::string_view script = "#";
    pegtl::memory_input in(script, "");
    REQUIRE(parseLiteral(in) == nullptr);
  }

  {
    std::string_view script = "##509a";
    pegtl::memory_input in(script, "");
    REQUIRE(parseLiteral(in) == nullptr);
  }

  {
    std::string_view script = "30915fab";
    pegtl::memory_input in(script, "");
    REQUIRE(parseLiteral(in) == nullptr);
  }

  {
    std::string_view script = "#000";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    REQUIRE(oo::isReference(*root->children[0], 0));
  }
}

TEST_CASE("can parse floating point literals", "[scripting]") {
  auto parseLiteral = [](auto &&in) {
    return pegtl::parse_tree::parse<oo::grammar::FloatLiteral,
                                    oo::AstNode,
                                    oo::AstSelector>(in);
  };

  {
    std::string_view script = "3.14159";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    REQUIRE(oo::isFloat(*root->children[0], 3.14159f));
  }

  {
    std::string_view script = "0.142";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    REQUIRE(oo::isFloat(*root->children[0], 0.142f));
  }

  {
    std::string_view script = "0.0001";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    REQUIRE(oo::isFloat(*root->children[0], 0.0001f));
  }

  {
    std::string_view script = ".142";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    REQUIRE(oo::isFloat(*root->children[0], 0.142f));
  }

  {
    std::string_view script = ".0001";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    REQUIRE(oo::isFloat(*root->children[0], 0.0001f));
  }

  {
    std::string_view script = "01.32";
    pegtl::memory_input in(script, "");
    REQUIRE(parseLiteral(in) == nullptr);
  }

  {
    std::string_view script = ".";
    pegtl::memory_input in(script, "");
    REQUIRE(parseLiteral(in) == nullptr);
  }

  {
    std::string_view script = "3.1.4";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    REQUIRE(oo::isFloat(*root->children[0], 3.1f));
  }
}

TEST_CASE("can parse literals", "[scripting]") {
  auto parseLiteral = [](auto &&in) {
    return pegtl::parse_tree::parse<oo::grammar::Literal,
                                    oo::AstNode,
                                    oo::AstSelector>(in);
  };

  {
    std::string_view script = "3.14";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    REQUIRE(root->children[0]->is<oo::grammar::FloatLiteral>());
  }

  {
    std::string_view script = "359";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    REQUIRE(root->children[0]->is<oo::grammar::IntegerLiteral>());
  }

  {
    std::string_view script = "0";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    REQUIRE(root->children[0]->is<oo::grammar::IntegerLiteral>());
  }

  {
    std::string_view script = R"("359")";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    REQUIRE(root->children[0]->is<oo::grammar::StringLiteralContents>());
  }
  {
    std::string_view script = "#59";
    pegtl::memory_input in(script, "");
    const auto root = parseLiteral(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    REQUIRE(root->children[0]->is<oo::grammar::RefLiteralContents>());
  }
}

TEST_CASE("can parse expressions", "[scripting]") {
  auto parseExpression = [](auto &&in) {
    return pegtl::parse_tree::parse<oo::grammar::Expression,
                                    oo::AstNode,
                                    oo::AstSelector>(in);
  };

  {
    std::string_view script = "3.75";
    pegtl::memory_input in(script, "");
    const auto root = parseExpression(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());
    REQUIRE(root->children[0]->is<oo::grammar::FloatLiteral>());
  }

  {
    std::string_view script = "-75";
    pegtl::memory_input in(script, "");
    const auto root = parseExpression(in);
    REQUIRE(root != nullptr);
    REQUIRE(!root->children.empty());

    const auto &op{root->children[0]};
    REQUIRE(op->is<oo::grammar::UnaryOperator>());
    REQUIRE(op->getValue() == "-");
    REQUIRE_FALSE(op->children.empty());

    const auto &literal{op->children[0]};
    REQUIRE(literal->is<oo::grammar::IntegerLiteral>());
  }

  {
    std::string_view script = "+75";
    pegtl::memory_input in(script, "");
    const auto root = parseExpression(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());

    const auto &op{root->children[0]};
    REQUIRE(op->is<oo::grammar::UnaryOperator>());
    REQUIRE(op->getValue() == "+");
    REQUIRE_FALSE(op->children.empty());

    const auto &literal{op->children[0]};
    REQUIRE(literal->is<oo::grammar::IntegerLiteral>());
  }

  {
    std::string_view script = "(75)";
    pegtl::memory_input in(script, "");
    const auto root = parseExpression(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());

    const auto &literal{root->children[0]};
    REQUIRE(literal->is<oo::grammar::IntegerLiteral>());
  }

  {
    std::string_view script = "(  ( (((75) ))  ))";
    pegtl::memory_input in(script, "");
    const auto root = parseExpression(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());

    const auto &literal{root->children[0]};
    REQUIRE(literal->is<oo::grammar::IntegerLiteral>());
  }

  {
    std::string_view script = "39 || -75";
    pegtl::memory_input in(script, "");
    const auto root = parseExpression(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());

    const auto &op{root->children[0]};
    REQUIRE(op->is<oo::grammar::BinaryOperator>());
    REQUIRE(op->getValue() == "||");
    REQUIRE(op->children.size() == 2);

    const auto &literal{op->children[0]};
    REQUIRE(literal->is<oo::grammar::IntegerLiteral>());

    const auto &subExpr{op->children[1]};
    REQUIRE(subExpr->is<oo::grammar::UnaryOperator>());
    REQUIRE(subExpr->getValue() == "-");
  }

  {
    std::string_view script = "1 && 2 || 3 && 4";
    pegtl::memory_input in(script, "");
    const auto root = parseExpression(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());

    const auto &orOp{root->children[0]};
    REQUIRE(orOp->is<oo::grammar::BinaryOperator>());
    REQUIRE(orOp->getValue() == "||");
    REQUIRE(orOp->children.size() == 2);

    const auto &lhs{orOp->children[0]};
    REQUIRE(lhs->is<oo::grammar::BinaryOperator>());
    REQUIRE(lhs->getValue() == "&&");

    const auto &rhs{orOp->children[1]};
    REQUIRE(rhs->is<oo::grammar::BinaryOperator>());
    REQUIRE(rhs->getValue() == "&&");
  }

  {
    std::string_view script = "(1 || 2) && (3 || 4)";
    pegtl::memory_input in(script, "");
    const auto root = parseExpression(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());

    const auto &andOp{root->children[0]};
    REQUIRE(andOp->is<oo::grammar::BinaryOperator>());
    REQUIRE(andOp->getValue() == "&&");
    REQUIRE(andOp->children.size() == 2);

    const auto &lhs{andOp->children[0]};
    REQUIRE(lhs->is<oo::grammar::BinaryOperator>());
    REQUIRE(lhs->getValue() == "||");

    const auto &rhs{andOp->children[1]};
    REQUIRE(rhs->is<oo::grammar::BinaryOperator>());
    REQUIRE(rhs->getValue() == "||");
  }

  {
    std::string_view script = "2 * 3 / 4 * 2";
    pegtl::memory_input in(script, "");
    const auto root = parseExpression(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());

    const auto &mulOp{root->children[0]};
    REQUIRE(mulOp->is<oo::grammar::BinaryOperator>());
    REQUIRE(mulOp->getValue() == "*");
    REQUIRE(mulOp->children.size() == 2);

    const auto &divOp{mulOp->children[0]};
    REQUIRE(divOp->is<oo::grammar::BinaryOperator>());
    REQUIRE(divOp->getValue() == "/");
    REQUIRE(divOp->children.size() == 2);

    const auto &mulOp2{divOp->children[0]};
    REQUIRE(mulOp2->is<oo::grammar::BinaryOperator>());
    REQUIRE(mulOp2->getValue() == "*");
    REQUIRE(mulOp2->children.size() == 2);

    REQUIRE(oo::isInteger(*mulOp2->children[0], 2));
    REQUIRE(oo::isInteger(*mulOp2->children[1], 3));
    REQUIRE(oo::isInteger(*divOp->children[1], 4));
    REQUIRE(oo::isInteger(*mulOp->children[1], 2));
  }

  {
    std::string_view script = "1 + 2 * 3";
    pegtl::memory_input in(script, "");
    const auto root = parseExpression(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());

    const auto &plusOp{root->children[0]};
    REQUIRE(plusOp->is<oo::grammar::BinaryOperator>());
    REQUIRE(plusOp->getValue() == "+");
    REQUIRE(plusOp->children.size() == 2);

    REQUIRE(oo::isInteger(*plusOp->children[0], 1));

    const auto &mulOp{plusOp->children[1]};
    REQUIRE(mulOp->is<oo::grammar::BinaryOperator>());
    REQUIRE(mulOp->getValue() == "*");
    REQUIRE(mulOp->children.size() == 2);

    REQUIRE(oo::isInteger(*mulOp->children[0], 2));
    REQUIRE(oo::isInteger(*mulOp->children[1], 3));
  }

  {
    std::string_view script = "1 && 2 * 3 <= 4";
    pegtl::memory_input in(script, "");
    const auto root = parseExpression(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());

    const auto &andOp{root->children[0]};
    REQUIRE(andOp->is<oo::grammar::BinaryOperator>());
    REQUIRE(andOp->getValue() == "&&");
    REQUIRE(andOp->children.size() == 2);

    REQUIRE(oo::isInteger(*andOp->children[0], 1));

    const auto &lteqOp{andOp->children[1]};
    REQUIRE(lteqOp->is<oo::grammar::BinaryOperator>());
    REQUIRE(lteqOp->getValue() == "<=");
    REQUIRE(lteqOp->children.size() == 2);

    const auto &mulOp{lteqOp->children[0]};
    REQUIRE(mulOp->is<oo::grammar::BinaryOperator>());
    REQUIRE(mulOp->getValue() == "*");
    REQUIRE(mulOp->children.size() == 2);

    REQUIRE(oo::isInteger(*mulOp->children[0], 2));
    REQUIRE(oo::isInteger(*mulOp->children[1], 3));
    REQUIRE(oo::isInteger(*lteqOp->children[1], 4));
  }

  {
    std::string_view script = "3 != 4";
    pegtl::memory_input in(script, "");
    const auto root = parseExpression(in);
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->children.empty());

    const auto &neqOp{root->children[0]};
    REQUIRE(neqOp->is<oo::grammar::BinaryOperator>());
    REQUIRE(neqOp->getValue() == "!=");
    REQUIRE(neqOp->children.size() == 2);

    REQUIRE(oo::isInteger(*neqOp->children[0], 3));
    REQUIRE(oo::isInteger(*neqOp->children[1], 4));
  }
}

TEST_CASE("can declare and assign to variables", "[scripting]") {
  {
    std::string_view script = R"script(
scn MyScript
begin GameMode
    short v4r ; This is a short
      short    var9
    ref myRef

    ; It's pretty long
    long long ; looong
    short long;Yes, you can use keywords as identifiers

    float
       f
end
    )script";
    pegtl::memory_input in(script, "");
    const auto root{oo::parseScript(in)};
    REQUIRE(root != nullptr);
    REQUIRE(root->children.size() == 2);

    const auto &block{root->children[1]};
    REQUIRE(block->children.size() == 7);

    REQUIRE(oo::isVariable<oo::grammar::RawShort>(*block->children[1], "v4r"));
    REQUIRE(oo::isVariable<oo::grammar::RawShort>(*block->children[2], "var9"));
    REQUIRE(oo::isVariable<oo::grammar::RawRef>(*block->children[3], "myRef"));
    REQUIRE(oo::isVariable<oo::grammar::RawLong>(*block->children[4], "long"));
    REQUIRE(oo::isVariable<oo::grammar::RawShort>(*block->children[5], "long"));
    REQUIRE(oo::isVariable<oo::grammar::RawFloat>(*block->children[6], "f"));
  }

  {
    std::string_view script = R"script(
scn MyScript
short glob1
begin GameMode
end float glob2
begin MenuMode long loc1
end
short noUse
    )script";

    pegtl::memory_input in(script, "");
    const auto root{oo::parseScript(in)};
    REQUIRE(root != nullptr);
    REQUIRE(root->children.size() == 6);

    REQUIRE(oo::isVariable<oo::grammar::RawShort>(*root->children[1], "glob1"));
    REQUIRE(oo::isVariable<oo::grammar::RawFloat>(*root->children[3], "glob2"));
    REQUIRE(oo::isVariable<oo::grammar::RawShort>(*root->children[5], "noUse"));

    const auto &block{root->children[4]};
    REQUIRE(block->children.size() == 2);
    REQUIRE(oo::isVariable<oo::grammar::RawLong>(*block->children[1], "loc1"));
  }

  {
    std::string_view script = R"script(
scn MyScript
begin GameMode
float short
short float
set float  to    3 ; Totally not confusing
set short to3.5 ; Isn't this language great?
end
    )script";

    pegtl::memory_input in(script, "");
    const auto root{oo::parseScript(in)};
    REQUIRE(root != nullptr);
    REQUIRE(root->children.size() == 2);

    const auto &block{root->children[1]};
    REQUIRE(block->children.size() == 5);

    REQUIRE(oo::isVariable<oo::grammar::RawFloat>(*block->children[1],
                                                  "short"));
    REQUIRE(oo::isVariable<oo::grammar::RawShort>(*block->children[2],
                                                  "float"));

    const auto &set1{block->children[3]};
    REQUIRE(set1->is<oo::grammar::SetStatement>());
    REQUIRE(set1->children.size() == 2);

    const auto &set1Name{set1->children[0]};
    REQUIRE(set1Name->has_content());
    REQUIRE(set1Name->content() == "float");
    REQUIRE(oo::isInteger(*set1->children[1], 3));

    const auto &set2{block->children[4]};
    REQUIRE(set2->is<oo::grammar::SetStatement>());
    REQUIRE(set2->children.size() == 2);

    const auto &set2Name{set2->children[0]};
    REQUIRE(set2Name->has_content());
    REQUIRE(set2Name->content() == "short");
    REQUIRE(oo::isFloat(*set2->children[1], 3.5f));
  }

  {
    std::string_view script = R"script(
scn MyScript
begin GameMode
set SomeQuest.foo to SomeQuest.foo * 2
set #001234ab.bar to 8
end
    )script";

    pegtl::memory_input in(script, "");
    const auto root{oo::parseScript(in)};
    REQUIRE(root != nullptr);
    REQUIRE(root->children.size() == 2);

    const auto &block{root->children[1]};
    REQUIRE(block->children.size() == 3);

    const auto &set1{block->children[1]};
    REQUIRE(set1->is<oo::grammar::SetStatement>());
    REQUIRE(set1->children.size() == 2);

    const auto &set1Dest{set1->children[0]};
    REQUIRE(set1Dest->is<oo::grammar::RawMemberAccess>());
    REQUIRE(set1Dest->children.size() == 2);

    REQUIRE(set1Dest->children[0]->is<oo::grammar::RawIdentifier>());
    REQUIRE(set1Dest->children[0]->has_content());
    REQUIRE(set1Dest->children[0]->content() == "SomeQuest");

    REQUIRE(set1Dest->children[1]->is<oo::grammar::RawIdentifier>());
    REQUIRE(set1Dest->children[1]->has_content());
    REQUIRE(set1Dest->children[1]->content() == "foo");

    const auto &set1Src{set1->children[1]};
    REQUIRE(set1Src->is<oo::grammar::BinaryOperator>());
    REQUIRE(set1Src->getValue() == "*");
    REQUIRE(set1Src->children.size() == 2);
    REQUIRE(oo::isInteger(*set1Src->children[1], 2));

    const auto &set1SrcVar{set1Src->children[0]};
    REQUIRE(set1SrcVar->is<oo::grammar::RawMemberAccess>());
    REQUIRE(set1SrcVar->children.size() == 2);

    REQUIRE(set1SrcVar->children[0]->is<oo::grammar::RawIdentifier>());
    REQUIRE(set1SrcVar->children[0]->has_content());
    REQUIRE(set1SrcVar->children[0]->content() == "SomeQuest");

    REQUIRE(set1SrcVar->children[1]->is<oo::grammar::RawIdentifier>());
    REQUIRE(set1SrcVar->children[1]->has_content());
    REQUIRE(set1SrcVar->children[1]->content() == "foo");

    const auto &set2{block->children[2]};
    REQUIRE(set2->is<oo::grammar::SetStatement>());
    REQUIRE(set2->children.size() == 2);

    const auto &set2Dest{set2->children[0]};
    REQUIRE(set2Dest->is<oo::grammar::RawMemberAccess>());
    REQUIRE(set2Dest->children.size() == 2);

    REQUIRE(set2Dest->children[0]->is<oo::grammar::RefLiteralContents>());
    REQUIRE(set2Dest->children[0]->has_content());
    REQUIRE(set2Dest->children[0]->content() == "001234ab");

    REQUIRE(set2Dest->children[1]->is<oo::grammar::RawIdentifier>());
    REQUIRE(set2Dest->children[1]->has_content());
    REQUIRE(set2Dest->children[1]->content() == "bar");

    REQUIRE(oo::isInteger(*set2->children[1], 8));
  }
}

TEST_CASE("can explicitly return from blocks", "[scripting]") {
  std::string_view script = R"script(
scn MyScript
begin GameMode
  return
  float foo
end

begin MenuMode
  return 7.5 + 10
end
  )script";

  pegtl::memory_input in(script, "");
  const auto root{oo::parseScript(in)};
  REQUIRE(root != nullptr);
  REQUIRE(root->children.size() == 3);

  const auto &gameBlock{root->children[1]};
  REQUIRE(gameBlock->children.size() == 3);

  const auto &gameRet{gameBlock->children[1]};
  REQUIRE(gameRet->is<oo::grammar::ReturnStatement>());
  REQUIRE(gameRet->children.empty());

  const auto &menuBlock{root->children[2]};
  REQUIRE(menuBlock->children.size() == 2);

  const auto &menuRet{menuBlock->children[1]};
  REQUIRE(menuRet->is<oo::grammar::ReturnStatement>());
  REQUIRE(menuRet->children.size() == 1);
}

TEST_CASE("can call free functions", "[scripting]") {
  {
    std::string_view script = R"script(
scn MyScript
begin GameMode
  set foo to MyFunc 124 3.14 #01abcdef someArg
end
    )script";
    pegtl::memory_input in(script, "");
    const auto root{oo::parseScript(in)};

    REQUIRE(root != nullptr);
    REQUIRE(root->children.size() == 2);
    const auto &blockStatement{root->children[1]};
    REQUIRE(blockStatement->children.size() == 2);

    const auto &setStatement{blockStatement->children[1]};
    REQUIRE(setStatement->children.size() == 2);

    const auto &src{setStatement->children[1]};
    REQUIRE(src->is<oo::grammar::RawCall>());
    REQUIRE(src->getValue() == "MyFunc");
    REQUIRE(src->children.size() == 4);

    REQUIRE(oo::isInteger(*src->children[0], 124));
    REQUIRE(oo::isFloat(*src->children[1], 3.14f));
    REQUIRE(oo::isReference(*src->children[2], 0x01abcdef));
    REQUIRE(src->children[3]->is<oo::grammar::RawIdentifier>());
  }

  {
    std::string_view script = R"script(
scn MyScript
begin GameMode
  set foo to Func1 10 * Func2 30
end
    )script";
    pegtl::memory_input in(script, "");
    const auto root{oo::parseScript(in)};

    REQUIRE(root != nullptr);
    REQUIRE(root->children.size() == 2);
    const auto &blockStatement{root->children[1]};
    REQUIRE(blockStatement->children.size() == 2);

    const auto &setStatement{blockStatement->children[1]};
    REQUIRE(setStatement->children.size() == 2);

    const auto &src{setStatement->children[1]};
    REQUIRE(src->is<oo::grammar::BinaryOperator>());
    REQUIRE(src->children.size() == 2);

    const auto &lhs{src->children[0]};
    REQUIRE(lhs->is<oo::grammar::RawCall>());
    REQUIRE(lhs->getValue() == "Func1");
    REQUIRE(lhs->children.size() == 1);
    REQUIRE(oo::isInteger(*lhs->children[0], 10));

    const auto &rhs{src->children[1]};
    REQUIRE(rhs->is<oo::grammar::RawCall>());
    REQUIRE(rhs->getValue() == "Func2");
    REQUIRE(rhs->children.size() == 1);
    REQUIRE(oo::isInteger(*rhs->children[0], 30));
  }

  {
    std::string_view script = R"script(
scn MyScript
begin GameMode
  set foo to Func1 Global.Value
end
)script";
    pegtl::memory_input in(script, "");
    const auto root{oo::parseScript(in)};
    REQUIRE(root != nullptr);
  }
}

TEST_CASE("can call member functions", "[scripting]") {
  {
    std::string_view script = R"script(
scn MyScript
begin GameMode
  set foo to obj.Func 10
end
    )script";
    pegtl::memory_input in(script, "");
    const auto root{oo::parseScript(in)};

    REQUIRE(root != nullptr);
    REQUIRE(root->children.size() == 2);
    const auto &blockStatement{root->children[1]};
    REQUIRE(blockStatement->children.size() == 2);

    const auto &setStatement{blockStatement->children[1]};
    REQUIRE(setStatement->children.size() == 2);

    const auto &src{setStatement->children[1]};
    REQUIRE(src->is<oo::grammar::RawCall>());
    REQUIRE(src->getValue() == "Func");
    REQUIRE(src->children.size() == 2);
    REQUIRE(src->children[0]->is<oo::grammar::RawIdentifier>());
    REQUIRE(oo::isInteger(*src->children[1], 10));
  }
}