#include "meta.hpp"
#include <catch2/catch.hpp>

TEST_CASE("Can append to variants", "[meta]") {
  {
    using src = std::variant<int, float>;
    using expected = std::variant<int, float, std::string>;
    STATIC_REQUIRE(std::is_same_v<variant_with<src, std::string>, expected>);
  }

  {
    using src = std::variant<int>;
    using expected = std::variant<int, const int>;
    STATIC_REQUIRE(std::is_same_v<variant_with<src, const int>, expected>);
  }
}

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

TEST_CASE("try_functor finds existing functions", "[meta]") {
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

TEST_CASE("try_functor defaults for nonexistent functions", "[meta]") {
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

TEST_CASE("try_predicate finds existing functions", "[meta]") {
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

TEST_CASE("try_predicate defaults for nonexistent functions", "[meta]") {
  less_than_predicate f{};
  const bool b1{false};
  const bool b2{true};
  const auto r1{try_predicate<bool>(f, &b1, &b2)};
  REQUIRE_FALSE(r1);
}
