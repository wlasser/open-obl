#include "gui/stack/instructions.hpp"
#include "meta.hpp"
#include "gui/stack/program.hpp"
#include "gui/stack/types.hpp"
#include <catch2/catch.hpp>
#include <pugixml.hpp>
#include <cmath>
#include <sstream>
#include <string>
#include <variant>

namespace gui {

TEST_CASE("parseValueType deduces bool", "[gui][gui/stack]") {
  {
    const auto r{stack::parseValueType("&true;")};
    REQUIRE(std::holds_alternative<bool>(r));
    REQUIRE(std::get<bool>(r));
  }

  {
    const auto r{stack::parseValueType("&false;")};
    REQUIRE(std::holds_alternative<bool>(r));
    REQUIRE_FALSE(std::get<bool>(r));
  }

  {
    const auto r{stack::parseValueType("true")};
    REQUIRE_FALSE(std::holds_alternative<bool>(r));
  }

  {
    const auto r{stack::parseValueType("1")};
    REQUIRE_FALSE(std::holds_alternative<bool>(r));
  }
}

TEST_CASE("parseValueType deduces float", "[gui][gui/stack]") {
  {
    const auto r{stack::parseValueType("1")};
    REQUIRE(std::holds_alternative<float>(r));
    REQUIRE(std::get<float>(r) == 1);
  }

  {
    const auto r{stack::parseValueType("-10")};
    REQUIRE(std::holds_alternative<float>(r));
    REQUIRE(std::get<float>(r) == -10);
  }

  {
    const auto r{stack::parseValueType("0")};
    REQUIRE(std::holds_alternative<float>(r));
    REQUIRE(std::get<float>(r) == 0);
  }
  {
    const auto r{stack::parseValueType("3.14")};
    REQUIRE(std::holds_alternative<float>(r));
    REQUIRE_THAT(std::get<float>(r), Catch::WithinULP(3.14f, 1));
  }

  {
    const auto r{stack::parseValueType("-2.0")};
    REQUIRE(std::holds_alternative<float>(r));
    REQUIRE_THAT(std::get<float>(r), Catch::WithinULP(-2.0f, 1));
  }
}

TEST_CASE("parseValueType ignores trailing and leading whitespace",
          "[gui][gui/stack]") {
  {
    const auto r{stack::parseValueType("  123")};
    REQUIRE(std::holds_alternative<float>(r));
    REQUIRE(std::get<float>(r) == 123);
  }

  {
    const auto r{stack::parseValueType("123    ")};
    REQUIRE(std::holds_alternative<float>(r));
    REQUIRE(std::get<float>(r) == 123);
  }

  {
    const auto r{stack::parseValueType("  123     ")};
    REQUIRE(std::holds_alternative<float>(r));
    REQUIRE(std::get<float>(r) == 123);
  }

  {
    const auto r{stack::parseValueType("  hello  world ")};
    REQUIRE(std::holds_alternative<std::string>(r));
    REQUIRE(std::get<std::string>(r) == "hello  world");
  }

  {
    const auto r{stack::parseValueType("   ")};
    REQUIRE(std::holds_alternative<std::string>(r));
    REQUIRE(std::get<std::string>(r).empty());
  }

  {
    const auto r{stack::parseValueType("")};
    REQUIRE(std::holds_alternative<std::string>(r));
    REQUIRE(std::get<std::string>(r).empty());
  }
}

TEST_CASE("can push values onto the stack", "[gui][gui/stack]") {
  stack::Program program{};

  SECTION("hardcoded program") {
    program.instructions = std::vector<stack::Instruction>{
        stack::push_t{10.0f},
        stack::push_t{15.0f}
    };
  }

  SECTION("from XML") {
    std::istringstream is{R"xml(
<copy>10</copy>
<copy>15</copy>
    )xml"};
    pugi::xml_document doc{};
    REQUIRE(doc.load(is));
    program = stack::compile(doc.root());
  }

  auto ret{program()};
  REQUIRE(std::holds_alternative<float>(ret));
  REQUIRE(std::get<float>(ret) == 15);
}

TEST_CASE("can perform integer arithmetic on the stack", "[gui][gui/stack]") {
  stack::Program program{};

  SECTION("hardcoded program") {
    program.instructions = std::vector<stack::Instruction>{
        stack::push_t{5},
        stack::push_t{10},
        stack::add_t{},
        stack::push_t{3},
        stack::div_t{},
        stack::push_t{2},
        stack::mul_t{},
        stack::push_t{3},
        stack::mod_t{}
    };

    auto ret{program()};
    REQUIRE(std::holds_alternative<int>(ret));
    REQUIRE(std::get<int>(ret) == ((5 + 10) / 3 * 2) % 3);
  }

  SECTION("from XML") {
    std::istringstream is{R"xml(
<copy>5</copy>
<add>10</add>
<div>3</div>
<mul>2</mul>
<mod>3</mod>
    )xml"};
    pugi::xml_document doc{};
    REQUIRE(doc.load(is));
    program = stack::compile(doc.root());

    auto ret{program()};
    REQUIRE(std::holds_alternative<float>(ret));
    REQUIRE(std::get<float>(ret)
                == std::fmod(((5.0f + 10.0f) / 3.0f * 2.0f), 3.0f));
  }
}

TEST_CASE("can perform floating point arithmetic on the stack",
          "[gui][gui/stack") {
  stack::Program program{};

  SECTION("hardcoded program") {
    program.instructions = std::vector<stack::Instruction>{
        stack::push_t{3.1f},
        stack::push_t{6.9f},
        stack::add_t{},
        stack::push_t{2.5f},
        stack::div_t{},
        stack::push_t{0.7f},
        stack::mul_t{}
    };
  }

  SECTION("from XML") {
    std::istringstream is{R"xml(
<copy>3.1</copy>
<add>6.9</add>
<div>2.5</div>
<mul>0.7</mul>
    )xml"};
    pugi::xml_document doc{};
    REQUIRE(doc.load(is));
    program = stack::compile(doc.root());
  }

  auto ret{program()};
  REQUIRE(std::holds_alternative<float>(ret));
  REQUIRE_THAT(std::get<float>(ret),
               Catch::WithinULP((3.1f + 6.9f) / 2.5f * 0.7f, 1));
}

TEST_CASE("can nop and retain stack state", "[gui][gui/stack]") {
  stack::Program program{};
  program.instructions = std::vector<stack::Instruction>{
      stack::push_t{10},
      stack::nop_t{},
      stack::nop_t{},
      stack::push_t{5},
      stack::nop_t{},
      stack::add_t{},
      stack::nop_t{}
  };

  auto ret{program()};
  REQUIRE(std::holds_alternative<int>(ret));
  REQUIRE(std::get<int>(ret) == (10 + 5));
}

TEST_CASE("can compute gcd and lcm on the stack", "[gui][gui/stack]") {
  stack::Program p_not_coprime{};
  stack::Program p_coprime{};
  stack::Program p_lcm{};

  SECTION("hardcoded program") {
    p_not_coprime.instructions = std::vector<stack::Instruction>{
        stack::push_t{100},
        stack::push_t{128},
        stack::gcd_t{}
    };

    p_coprime.instructions = std::vector<stack::Instruction>{
        stack::push_t{79},
        stack::push_t{25},
        stack::gcd_t{}
    };

    p_lcm.instructions = std::vector<stack::Instruction>{
        stack::push_t{100},
        stack::push_t{128},
        stack::lcm_t{}
    };

    auto r_not_coprime{p_not_coprime()};
    REQUIRE(std::holds_alternative<int>(r_not_coprime));
    REQUIRE(std::get<int>(r_not_coprime) == 4);

    auto r_coprime{p_coprime()};
    REQUIRE(std::holds_alternative<int>(r_coprime));
    REQUIRE(std::get<int>(r_coprime) == 1);

    auto r_lcm{p_lcm()};
    REQUIRE(std::holds_alternative<int>(r_lcm));
    REQUIRE(std::get<int>(r_lcm) == 128 * 25);
  }

  SECTION("from XML") {
    {
      std::istringstream is{R"xml(
<copy>100</copy>
<gcd>128</gcd>
      )xml"};
      pugi::xml_document doc{};
      REQUIRE(doc.load(is));
      p_not_coprime = stack::compile(doc.root());
    }

    {
      std::istringstream is{R"xml(
<copy>79</copy>
<gcd>25</gcd>
      )xml"};
      pugi::xml_document doc{};
      REQUIRE(doc.load(is));
      p_coprime = stack::compile(doc.root());
    }

    {
      std::istringstream is{R"xml(
<copy>100</copy>
<lcm>128</lcm>
      )xml"};
      pugi::xml_document doc{};
      REQUIRE(doc.load(is));
      p_lcm = stack::compile(doc.root());
    }

    auto r_not_coprime{p_not_coprime()};
    REQUIRE(std::holds_alternative<float>(r_not_coprime));
    REQUIRE(std::get<float>(r_not_coprime) == 4);

    auto r_coprime{p_coprime()};
    REQUIRE(std::holds_alternative<float>(r_coprime));
    REQUIRE(std::get<float>(r_coprime) == 1);

    auto r_lcm{p_lcm()};
    REQUIRE(std::holds_alternative<float>(r_lcm));
    REQUIRE(std::get<float>(r_lcm) == 128 * 25);
  }
}

TEST_CASE("can compute floor and ceiling on the stack", "[gui][gui/stack]") {
  stack::Program program{};

  SECTION("hardcoded program") {
    program.instructions = std::vector<stack::Instruction>{
        stack::push_t{7.5f},
        stack::push_t{0.0f},
        stack::floor_t{},
        stack::push_t{6.7f},
        stack::ceil_t{}
    };
  }

  SECTION("from XML") {
    std::istringstream is{R"xml(
<copy>7.5</copy>
<copy>0.0</copy>
<floor></floor>
<ceil>6.7</ceil>
    )xml"};
    pugi::xml_document doc{};
    REQUIRE(doc.load(is));
    program = stack::compile(doc.root());
  }

  auto ret{program()};
  REQUIRE(std::holds_alternative<float>(ret));
  REQUIRE_THAT(std::get<float>(ret), Catch::WithinULP(14.0f, 1));
}

TEST_CASE("can compute min and max on the stack", "[gui][gui/stack]") {
  stack::Program p_min{};
  stack::Program p_max{};

  SECTION("hardcoded program") {
    p_min.instructions = std::vector<stack::Instruction>{
        stack::push_t{6.5f},
        stack::push_t{3.9f},
        stack::min_t{}
    };

    p_max.instructions = std::vector<stack::Instruction>{
        stack::push_t{6.5f},
        stack::push_t{3.9f},
        stack::max_t{}
    };
  }

  SECTION("from XML") {
    {
      std::istringstream is{R"xml(
<copy>6.5</copy>
<copy>3.9</copy>
<min></min>
      )xml"};
      pugi::xml_document doc{};
      REQUIRE(doc.load(is));
      p_min = stack::compile(doc.root());
    }

    {
      std::istringstream is{R"xml(
<copy>6.5</copy>
<copy>3.9</copy>
<max></max>
      )xml"};
      pugi::xml_document doc{};
      REQUIRE(doc.load(is));
      p_max = stack::compile(doc.root());
    }
  }

  auto r_min{p_min()};
  REQUIRE(std::holds_alternative<float>(r_min));
  REQUIRE(std::get<float>(r_min) == 3.9f);

  auto r_max{p_max()};
  REQUIRE(std::holds_alternative<float>(r_max));
  REQUIRE(std::get<float>(r_max) == 6.5f);
}

TEST_CASE("can compute abs on the stack", "[gui][gui/stack]") {
  stack::Program program{};

  SECTION("hardcoded program") {
    program.instructions = std::vector<stack::Instruction>{
        stack::push_t{-7.5f},
        stack::push_t{1.0f},
        stack::abs_t{}
    };
  }

  SECTION("from XML") {
    std::istringstream is{R"xml(
<copy>-7.5</copy>
<abs>1.0</abs>
    )xml"};
    pugi::xml_document doc{};
    REQUIRE(doc.load(is));
    program = stack::compile(doc.root());
  }

  auto ret{program()};
  REQUIRE(std::holds_alternative<float>(ret));
  REQUIRE_THAT(std::get<float>(ret), Catch::WithinULP(6.5f, 1));
}

TEST_CASE("can do comparisons on the stack", "[gui][gui/stack]") {
  stack::Program program{};

  SECTION("hardcoded program") {
    program.instructions = std::vector<stack::Instruction>{
        stack::push_t{3},
        stack::push_t{5},
        stack::lt_t{},
        stack::push_t{"world"},
        stack::push_t{"hello"},
        stack::gt_t{},
        stack::push_t{3.1f},
        stack::push_t{3.2f},
        stack::eq_t{},
        stack::not_t{},
        stack::push_t{5},
        stack::push_t{3},
        stack::neq_t{},
        stack::and_t{},
        stack::and_t{},
        stack::and_t{}
    };
  }

  SECTION("from XML") {
    std::istringstream is{R"xml(
<copy>3</copy>
<lt>5</lt>
<copy>world</copy>
<copy>hello</copy>
<gt></gt>
<copy>3.1</copy>
<eq>3.2</eq>
<not></not>
<copy>5</copy>
<copy>3</copy>
<neq/>
<and/>
<and/>
<and/>
    )xml"};
    pugi::xml_document doc{};
    REQUIRE(doc.load(is));
    program = stack::compile(doc.root());
  }

  SECTION("from nested XML") {
    std::istringstream is{R"xml(
<copy>3</copy>
<lt>5</lt>

<and>
  <copy>world</copy>
  <gt>hello</gt>

  <and>
    <not>
      <copy>3.1</copy>
      <eq>3.2</eq>
    </not>

    <and>
      <copy>5</copy>
      <neq>3</neq>
    </and>
  </and>

</and>
    )xml"};
    pugi::xml_document doc{};
    REQUIRE(doc.load(is));
    program = stack::compile(doc.root());
  }

  auto ret{program()};
  REQUIRE(std::holds_alternative<bool>(ret));
  REQUIRE(std::get<bool>(ret));
}

TEST_CASE("can perform logical operations on the stack", "[gui][gui/stack]") {
  SECTION("binary operators") {
    stack::Program program{};

    SECTION("hardcoded program") {
      program.instructions = std::vector<stack::Instruction>{
          stack::push_t{true},
          stack::push_t{false},
          stack::or_t{},
          stack::push_t{true},
          stack::and_t{}
      };
    }

    SECTION("from XML") {
      std::istringstream is{R"xml(
<copy>&true;</copy>
<copy>&false;</copy>
<or/>
<and>&true;</and>
      )xml"};
      pugi::xml_document doc{};
      REQUIRE(doc.load(is));
      program = stack::compile(doc.root());
    }

    auto ret{program()};
    REQUIRE(std::holds_alternative<bool>(ret));
    REQUIRE(std::get<bool>(ret));
  }

  SECTION("unary operators") {
    stack::Program program{};

    SECTION("hardcoded program") {
      program.instructions = std::vector<stack::Instruction>{
          stack::push_t{false},
          stack::not_t{}
      };
    }

    SECTION("from XML") {
      std::istringstream is{R"xml(
<not>&false;</not>
      )xml"};
      pugi::xml_document doc{};
      REQUIRE(doc.load(is));
      program = stack::compile(doc.root());
    }

    auto ret{program()};
    REQUIRE(std::holds_alternative<bool>(ret));
    REQUIRE(std::get<bool>(ret));
  }
}

TEST_CASE("can branch on the stack", "[gui][gui/stack]") {
  stack::Program program{};

  SECTION("hardcoded program") {
    program.instructions = std::vector<stack::Instruction>{
        stack::push_t{0},    // Zero buffer
        stack::push_t{0},
        stack::push_t{3},    // Desired final value
        stack::push_t{5},
        stack::push_t{5},
        stack::eq_t{},       // == true
        stack::onlyif_t{},   // Should just be '3'
        stack::push_t{7},    // Want this to be discarded
        stack::push_t{5},
        stack::push_t{5},
        stack::eq_t{},       // == true
        stack::onlyifnot_t{} // Should discard the 'true' and '7'
    };

    auto ret{program()};
    REQUIRE(std::holds_alternative<int>(ret));
    REQUIRE(std::get<int>(ret) == 3);
  }

  SECTION("from XML") {
    std::istringstream is{R"xml(
<!-- Zero buffer -->
<copy>0</copy>
<copy>0</copy>

<copy>3</copy>
<onlyif>
  <copy>5</copy>
  <eq>5</eq>
</onlyif>

<copy>7</copy>
<onlyifnot>
  <copy>5</copy>
  <eq>5</eq>
</onlyifnot>
    )xml"};
    pugi::xml_document doc{};
    REQUIRE(doc.load(is));
    program = stack::compile(doc.root());

    auto ret{program()};
    REQUIRE(std::holds_alternative<float>(ret));
    REQUIRE(std::get<float>(ret) == 3);
  }

}

TEST_CASE("binary operators can act on stacks with one element",
          "[gui][gui/stack]") {
  {
    stack::Program program{};
    program.instructions = std::vector<stack::Instruction>{
        stack::push_t{1},
        stack::add_t{}
    };

    const auto ret{program()};
    REQUIRE(std::holds_alternative<int>(ret));
    REQUIRE(std::get<int>(ret) == 1);
  }

  {
    stack::Program program{};
    program.instructions = std::vector<stack::Instruction>{
        stack::push_t{3.0f},
        stack::add_t{}
    };

    const auto ret{program()};
    REQUIRE(std::holds_alternative<float>(ret));
    REQUIRE_THAT(std::get<float>(ret), Catch::WithinULP(3.0f, 1));
  }

  {
    stack::Program program{};
    program.instructions = std::vector<stack::Instruction>{
        stack::push_t{true},
        stack::or_t{}
    };

    const auto ret{program()};
    REQUIRE(std::holds_alternative<bool>(ret));
    REQUIRE(std::get<bool>(ret));
  }

  {
    stack::Program program{};
    program.instructions = std::vector<stack::Instruction>{
        stack::push_t{"Hello"},
        stack::add_t{}
    };

    const auto ret{program()};
    REQUIRE(std::holds_alternative<std::string>(ret));
    REQUIRE(std::get<std::string>(ret) == "Hello");
  }

}

TEST_CASE("empty programs throw on execution", "[gui][gui/stack]") {
  stack::Program program{};
  REQUIRE_THROWS_AS(program(), std::runtime_error);
}

} // namespace gui
