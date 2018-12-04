#include "gui/trait.hpp"
#include "gui/trait_selector.hpp"
#include <catch2/catch.hpp>

TEST_CASE("can convert trait type to TraitTypeId", "[gui]") {
  STATIC_REQUIRE(gui::getTraitTypeId<int>() == gui::TraitTypeId::Int);
  STATIC_REQUIRE(gui::getTraitTypeId<float>() == gui::TraitTypeId::Float);
  STATIC_REQUIRE(gui::getTraitTypeId<bool>() == gui::TraitTypeId::Bool);
  STATIC_REQUIRE(gui::getTraitTypeId<std::string>()
                     == gui::TraitTypeId::String);
  STATIC_REQUIRE(gui::getTraitTypeId<double>()
                     == gui::TraitTypeId::Unimplemented);
}

TEST_CASE("can get the index of user traits", "[gui]") {
  REQUIRE(gui::getUserTraitIndex("user0").value() == 0);
  REQUIRE(gui::getUserTraitIndex("user5").value() == 5);
  REQUIRE(gui::getUserTraitIndex("user123456789").value() == 123456789);
  REQUIRE_FALSE(gui::getUserTraitIndex("UsEr5").has_value());
  REQUIRE_FALSE(gui::getUserTraitIndex("user").has_value());
  REQUIRE_FALSE(gui::getUserTraitIndex("kitten3").has_value());
  REQUIRE_FALSE(gui::getUserTraitIndex("").has_value());
}

TEST_CASE("UserTraitInterface works", "[gui]") {
  {
    int t1{};
    std::string t2{};
    gui::UserTraitInterface<int, std::string> uti(std::make_tuple(&t1, &t2));
    REQUIRE(uti.userTraitType(0) == gui::TraitTypeId::Int);
    REQUIRE(uti.userTraitType(1) == gui::TraitTypeId::String);

    uti.set_user(0, 1);
    REQUIRE(t1 == 1);

    uti.set_user(1, std::string{"hello"});
    REQUIRE(t2 == "hello");

    uti.set_user(0, std::string{"hello"});
    REQUIRE(t1 == 1);
  }

  {
    // The fact that this compiles is a sufficient test
    constexpr gui::UserTraitInterface<> uti(std::tuple<>);
  }

  {
    int t1{};
    int t2{};
    float t3{};
    gui::UserTraitInterface<int, int, float>
        uti(std::make_tuple(&t1, &t2, &t3));

    uti.set_user(0, 1);
    REQUIRE(t1 == 1);

    uti.set_user(1, 2);
    REQUIRE(t2 == 2);
    REQUIRE(t1 == 1);

    uti.set_user(2, 3.5f);
    REQUIRE_THAT(t3, Catch::WithinULP(3.5f, 1));
  }
}

TEST_CASE("Can construct and call trait functions") {
  {
    gui::TraitFun<int> tf([]() { return 10; });
    REQUIRE(tf);
    REQUIRE(tf() == 10);
  }

  {
    gui::TraitFun<int> tf;
    REQUIRE_FALSE(tf);
  }

  {
    gui::TraitFun<std::string> tf([]() { return "hello"; });
    REQUIRE(tf);
    REQUIRE(tf() == "hello");
  }
}

TEST_CASE("can tokenize trait selectors", "[gui]") {
  {
    const auto ts{gui::tokenizeTraitSelector("child()")};
    REQUIRE(ts);
    REQUIRE(ts->type == gui::TraitSelector::Type::child);
    REQUIRE_FALSE(ts->argument);
  }

  {
    const auto ts{gui::tokenizeTraitSelector("last()")};
    REQUIRE(ts);
    REQUIRE(ts->type == gui::TraitSelector::Type::last);
    REQUIRE_FALSE(ts->argument);
  }

  {
    const auto ts{gui::tokenizeTraitSelector("me()")};
    REQUIRE(ts);
    REQUIRE(ts->type == gui::TraitSelector::Type::me);
    REQUIRE_FALSE(ts->argument);
  }

  {
    const auto ts{gui::tokenizeTraitSelector("parent()")};
    REQUIRE(ts);
    REQUIRE(ts->type == gui::TraitSelector::Type::parent);
    REQUIRE_FALSE(ts->argument);
  }

  {
    const auto ts{gui::tokenizeTraitSelector("screen()")};
    REQUIRE(ts);
    REQUIRE(ts->type == gui::TraitSelector::Type::screen);
    REQUIRE_FALSE(ts->argument);
  }

  {
    const auto ts{gui::tokenizeTraitSelector("sibling()")};
    REQUIRE(ts);
    REQUIRE(ts->type == gui::TraitSelector::Type::sibling);
    REQUIRE_FALSE(ts->argument);
  }

  {
    const auto ts{gui::tokenizeTraitSelector("strings()")};
    REQUIRE(ts);
    REQUIRE(ts->type == gui::TraitSelector::Type::strings);
    REQUIRE_FALSE(ts->argument);
  }

  {
    const auto ts{gui::tokenizeTraitSelector("foo()")};
    REQUIRE_FALSE(ts);
  }

  {
    const auto ts{gui::tokenizeTraitSelector("child(foo)")};
    REQUIRE(ts);
    REQUIRE(ts->type == gui::TraitSelector::Type::child);
    REQUIRE(ts->argument.value() == "foo");
  }
}
