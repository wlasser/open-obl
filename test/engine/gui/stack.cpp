#include "engine/gui/stack/instructions.hpp"
#include "engine/gui/stack/meta.hpp"
#include "engine/gui/stack/program.hpp"
#include "engine/gui/stack/types.hpp"
#include <catch.hpp>
#include <string>
#include <variant>

namespace engine::gui::stack {

namespace meta {

using int_or_float_t = std::variant<int, float>;
using int_or_float_or_string_t = std::variant<int, float, std::string>;

static_assert(std::is_same_v<variant_with<int_or_float_t, std::string>,
                             int_or_float_or_string_t>);

struct divide_functor {
  int operator()(int a, int b) const {
    return a / b;
  }
  float operator()(float a, float b) const {
    return a / b;
  }
  // For preventing implicit conversion of bool -> float or int
  bool operator()(bool, bool) const = delete;
};

TEST_CASE("try_functor finds existing functions",
          "[gui][gui/stack][gui/stack/meta]") {
  divide_functor f{};

  const int i1{1};
  const int i2{2};
  const auto i_result{try_functor<int>(f, &i1, &i2)};
  REQUIRE(i_result);
  REQUIRE(*i_result == f(i1, i2));

  const float f1{1.0f};
  const float f2{2.0f};
  const auto f_result{try_functor<float>(f, &f1, &f2)};
  REQUIRE(f_result);
  REQUIRE(*f_result == f(f1, f2));
}

TEST_CASE("try_functor defaults for nonexistent functions",
          "[gui][gui/stack][gui/stack/meta]") {
  divide_functor f{};
  const bool b1{false};
  const bool b2{true};
  const auto b_result{try_functor<bool>(f, &b1, &b2)};
  REQUIRE_FALSE(b_result);
}

struct less_than_predicate {
  bool operator()(int a, int b) const {
    return a < b;
  }
  bool operator()(float a, float b) const {
    return a < b;
  }
  bool operator()(bool a, bool b) const = delete;
  bool operator()(const std::string &a, const std::string &b) const {
    return a < b;
  }
};

TEST_CASE("try_predicate finds existing functions",
          "[gui][gui/stack][gui/stack/meta]") {
  less_than_predicate f{};

  const int i1{1};
  const int i2{2};
  const auto i_result{try_predicate<int>(f, &i1, &i2)};
  REQUIRE(i_result.has_value());
  REQUIRE(*i_result);

  const std::string s1{"Antelope"};
  const std::string s2{"Zebra"};
  const std::string s3{"Aardvark"};

  const auto s_result12{try_predicate<std::string>(f, &s1, &s2)};
  REQUIRE(s_result12.has_value());
  REQUIRE(*s_result12 == s1 < s2);

  const auto s_result13{try_predicate<std::string>(f, &s1, &s3)};
  REQUIRE(s_result13.has_value());
  REQUIRE(*s_result13 == s1 < s3);
}

TEST_CASE("try_predicate defaults for nonexistent functions",
          "[gui][gui/stack][gui/stack/meta]") {
  less_than_predicate f{};
  const bool b1{false};
  const bool b2{true};
  const auto r1{try_predicate<bool>(f, &b1, &b2)};
  REQUIRE_FALSE(r1);
}

} // namespace meta

TEST_CASE("parseValueType deduces bool", "[gui][gui/stack]") {
  const std::string str_true{"&true;"};
  const auto r_true{parseValueType(str_true)};
  REQUIRE(std::holds_alternative<bool>(r_true));
  REQUIRE(std::get<bool>(r_true));

  const std::string str_false{"&false;"};
  const auto r_false{parseValueType(str_false)};
  REQUIRE(std::holds_alternative<bool>(r_false));
  REQUIRE_FALSE(std::get<bool>(r_false));

  const std::string str_fake_true{"true"};
  const auto r_fake_true{parseValueType(str_fake_true)};
  REQUIRE_FALSE(std::holds_alternative<bool>(r_fake_true));

  const std::string str_int{"1"};
  const auto r_int{parseValueType(str_int)};
  REQUIRE_FALSE(std::holds_alternative<bool>(r_int));
}

TEST_CASE("parseValueType deduces int", "[gui][gui/stack]") {
  const std::string str_int_positive{"1"};
  const auto r_int_positive{parseValueType(str_int_positive)};
  REQUIRE(std::holds_alternative<int>(r_int_positive));
  REQUIRE(std::get<int>(r_int_positive) == 1);

  const std::string str_int_negative{"-10"};
  const auto r_int_negative{parseValueType(str_int_negative)};
  REQUIRE(std::holds_alternative<int>(r_int_negative));
  REQUIRE(std::get<int>(r_int_negative) == -10);

  const std::string str_int_zero{"0"};
  const auto r_int_zero{parseValueType(str_int_zero)};
  REQUIRE(std::holds_alternative<int>(r_int_zero));
  REQUIRE(std::get<int>(r_int_zero) == 0);
}

TEST_CASE("parseValueType deduces float", "[gui][gui/stack]") {
  const std::string str_float_positive{"3.14"};
  const auto r_float_positive{parseValueType(str_float_positive)};
  REQUIRE(std::holds_alternative<float>(r_float_positive));
  REQUIRE_THAT(std::get<float>(r_float_positive), Catch::WithinULP(3.14f, 1));

  const std::string str_float_negative{"-2.0"};
  const auto r_float_negative{parseValueType(str_float_negative)};
  REQUIRE(std::holds_alternative<float>(r_float_negative));
  REQUIRE_THAT(std::get<float>(r_float_negative), Catch::WithinULP(-2.0f, 1));
}

TEST_CASE("can push values onto the stack", "[gui][gui/stack]") {
  Program program{};
  program.instructions = std::vector<Instruction>{
      push_t{10},
      push_t{15}
  };

  auto ret{program()};
  REQUIRE(std::holds_alternative<int>(ret));
  REQUIRE(std::get<int>(ret) == 15);
}

TEST_CASE("can perform integer arithmetic on the stack", "[gui][gui/stack]") {
  Program program{};
  program.instructions = std::vector<Instruction>{
      push_t{5},
      push_t{10},
      add_t{},
      push_t{3},
      div_t{},
      push_t{2},
      mul_t{},
      push_t{3},
      mod_t{}
  };

  auto ret{program()};
  REQUIRE(std::holds_alternative<int>(ret));
  REQUIRE(std::get<int>(ret) == ((5 + 10) / 3 * 2) % 3);
}

TEST_CASE("can perform floating point arithmetic on the stack",
          "[gui][gui/stack") {
  Program program{};
  program.instructions = std::vector<Instruction>{
      push_t{3.1f},
      push_t{6.9f},
      add_t{},
      push_t{2.5f},
      div_t{},
      push_t{0.7f},
      mul_t{}
  };

  auto ret{program()};
  REQUIRE(std::holds_alternative<float>(ret));
  REQUIRE_THAT(std::get<float>(ret),
               Catch::WithinULP((3.1f + 6.9f) / 2.5f * 0.7f, 1));
}

TEST_CASE("can nop and retain stack state", "[gui][gui/stack]") {
  Program program{};
  program.instructions = std::vector<Instruction>{
      push_t{10},
      nop_t{},
      nop_t{},
      push_t{5},
      nop_t{},
      add_t{},
      nop_t{}
  };

  auto ret{program()};
  REQUIRE(std::holds_alternative<int>(ret));
  REQUIRE(std::get<int>(ret) == (10 + 5));
}

TEST_CASE("can compute gcd and lcm on the stack", "[gui][gui/stack]") {
  Program p_not_coprime{};
  p_not_coprime.instructions = std::vector<Instruction>{
      push_t{100},
      push_t{128},
      gcd_t{}
  };

  auto r_not_coprime{p_not_coprime()};
  REQUIRE(std::holds_alternative<int>(r_not_coprime));
  REQUIRE(std::get<int>(r_not_coprime) == 4);

  Program p_coprime{};
  p_coprime.instructions = std::vector<Instruction>{
      push_t{79},
      push_t{25},
      gcd_t{}
  };

  auto r_coprime{p_coprime()};
  REQUIRE(std::holds_alternative<int>(r_coprime));
  REQUIRE(std::get<int>(r_coprime) == 1);

  Program p_lcm{};
  p_lcm.instructions = std::vector<Instruction>{
      push_t{100},
      push_t{128},
      lcm_t{}
  };

  auto r_lcm{p_lcm()};
  REQUIRE(std::holds_alternative<int>(r_lcm));
  REQUIRE(std::get<int>(r_lcm) == 128 * 25);
}

TEST_CASE("can compute floor and ceiling on the stack", "[gui][gui/stack]") {
  Program program{};
  program.instructions = std::vector<Instruction>{
      push_t{7.5f},
      push_t{0.0f},
      floor_t{},
      push_t{6.7f},
      ceil_t{}
  };

  auto ret{program()};
  REQUIRE(std::holds_alternative<float>(ret));
  REQUIRE_THAT(std::get<float>(ret), Catch::WithinULP(14.0f, 1));
}

TEST_CASE("can compute min and max on the stack", "[gui][gui/stack]") {
  Program p_min{};
  p_min.instructions = std::vector<Instruction>{
      push_t{6.5f},
      push_t{3.9f},
      min_t{}
  };

  auto r_min{p_min()};
  REQUIRE(std::holds_alternative<float>(r_min));
  REQUIRE(std::get<float>(r_min) == 3.9f);

  Program p_max{};
  p_max.instructions = std::vector<Instruction>{
      push_t{6.5f},
      push_t{3.9f},
      max_t{}
  };

  auto r_max{p_max()};
  REQUIRE(std::holds_alternative<float>(r_max));
  REQUIRE(std::get<float>(r_max) == 6.5f);
}

TEST_CASE("can compute abs on the stack", "[gui][gui/stack]") {
  Program program{};
  program.instructions = std::vector<Instruction>{
      push_t{-7.5f},
      push_t{1.0f},
      abs_t{}
  };

  auto ret{program()};
  REQUIRE(std::holds_alternative<float>(ret));
  REQUIRE_THAT(std::get<float>(ret), Catch::WithinULP(6.5f, 1));
}

} // namespace engine::gui::stack
