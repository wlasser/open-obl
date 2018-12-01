#include "gui/stack/instructions.hpp"
#include "meta.hpp"
#include "gui/stack/program.hpp"
#include "gui/stack/types.hpp"
#include <catch2/catch.hpp>
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

TEST_CASE("parseValueType deduces int", "[gui][gui/stack]") {
  {
    const auto r{stack::parseValueType("1")};
    REQUIRE(std::holds_alternative<int>(r));
    REQUIRE(std::get<int>(r) == 1);
  }

  {
    const auto r{stack::parseValueType("-10")};
    REQUIRE(std::holds_alternative<int>(r));
    REQUIRE(std::get<int>(r) == -10);
  }

  {
    const auto r{stack::parseValueType("0")};
    REQUIRE(std::holds_alternative<int>(r));
    REQUIRE(std::get<int>(r) == 0);
  }
}

TEST_CASE("parseValueType deduces float", "[gui][gui/stack]") {
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

TEST_CASE("can push values onto the stack", "[gui][gui/stack]") {
  stack::Program program{};
  program.instructions = std::vector<stack::Instruction>{
      stack::push_t{10},
      stack::push_t{15}
  };

  auto ret{program()};
  REQUIRE(std::holds_alternative<int>(ret));
  REQUIRE(std::get<int>(ret) == 15);
}

TEST_CASE("can perform integer arithmetic on the stack", "[gui][gui/stack]") {
  stack::Program program{};
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

TEST_CASE("can perform floating point arithmetic on the stack",
          "[gui][gui/stack") {
  stack::Program program{};
  program.instructions = std::vector<stack::Instruction>{
      stack::push_t{3.1f},
      stack::push_t{6.9f},
      stack::add_t{},
      stack::push_t{2.5f},
      stack::div_t{},
      stack::push_t{0.7f},
      stack::mul_t{}
  };

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
  p_not_coprime.instructions = std::vector<stack::Instruction>{
      stack::push_t{100},
      stack::push_t{128},
      stack::gcd_t{}
  };

  auto r_not_coprime{p_not_coprime()};
  REQUIRE(std::holds_alternative<int>(r_not_coprime));
  REQUIRE(std::get<int>(r_not_coprime) == 4);

  stack::Program p_coprime{};
  p_coprime.instructions = std::vector<stack::Instruction>{
      stack::push_t{79},
      stack::push_t{25},
      stack::gcd_t{}
  };

  auto r_coprime{p_coprime()};
  REQUIRE(std::holds_alternative<int>(r_coprime));
  REQUIRE(std::get<int>(r_coprime) == 1);

  stack::Program p_lcm{};
  p_lcm.instructions = std::vector<stack::Instruction>{
      stack::push_t{100},
      stack::push_t{128},
      stack::lcm_t{}
  };

  auto r_lcm{p_lcm()};
  REQUIRE(std::holds_alternative<int>(r_lcm));
  REQUIRE(std::get<int>(r_lcm) == 128 * 25);
}

TEST_CASE("can compute floor and ceiling on the stack", "[gui][gui/stack]") {
  stack::Program program{};
  program.instructions = std::vector<stack::Instruction>{
      stack::push_t{7.5f},
      stack::push_t{0.0f},
      stack::floor_t{},
      stack::push_t{6.7f},
      stack::ceil_t{}
  };

  auto ret{program()};
  REQUIRE(std::holds_alternative<float>(ret));
  REQUIRE_THAT(std::get<float>(ret), Catch::WithinULP(14.0f, 1));
}

TEST_CASE("can compute min and max on the stack", "[gui][gui/stack]") {
  stack::Program p_min{};
  p_min.instructions = std::vector<stack::Instruction>{
      stack::push_t{6.5f},
      stack::push_t{3.9f},
      stack::min_t{}
  };

  auto r_min{p_min()};
  REQUIRE(std::holds_alternative<float>(r_min));
  REQUIRE(std::get<float>(r_min) == 3.9f);

  stack::Program p_max{};
  p_max.instructions = std::vector<stack::Instruction>{
      stack::push_t{6.5f},
      stack::push_t{3.9f},
      stack::max_t{}
  };

  auto r_max{p_max()};
  REQUIRE(std::holds_alternative<float>(r_max));
  REQUIRE(std::get<float>(r_max) == 6.5f);
}

TEST_CASE("can compute abs on the stack", "[gui][gui/stack]") {
  stack::Program program{};
  program.instructions = std::vector<stack::Instruction>{
      stack::push_t{-7.5f},
      stack::push_t{1.0f},
      stack::abs_t{}
  };

  auto ret{program()};
  REQUIRE(std::holds_alternative<float>(ret));
  REQUIRE_THAT(std::get<float>(ret), Catch::WithinULP(6.5f, 1));
}

TEST_CASE("can do comparisons on the stack", "[gui][gui/stack]") {
  stack::Program program{};
  program.instructions = std::vector<stack::Instruction>{
      stack::push_t{3},
      stack::push_t{5},
      stack::lt_t{},
      stack::push_t{std::string{"world"}},
      stack::push_t{std::string{"hello"}},
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

  auto ret{program()};
  REQUIRE(std::holds_alternative<bool>(ret));
  REQUIRE(std::get<bool>(ret));
}

TEST_CASE("can branch on the stack", "[gui][gui/stack]") {
  stack::Program program{};
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

} // namespace gui
