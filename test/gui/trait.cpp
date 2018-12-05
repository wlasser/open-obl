#include "gui/screen.hpp"
#include "gui/strings.hpp"
#include "gui/trait.hpp"
#include "gui/trait_selector.hpp"
#include "settings.hpp"
#include <catch2/catch.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <sstream>

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

std::shared_ptr<spdlog::logger> getLogger() {
  if (auto logger{spdlog::get(oo::LOG)}) return logger;
  return spdlog::stderr_color_mt(oo::LOG);
}

TEST_CASE("can use StringsElement", "[gui]") {
  [[maybe_unused]] auto logger{getLogger()};
  const std::string prefix{gui::StringsElement::getPrefix()};

  std::istringstream is{R"xml(
<rect name="Strings">
  <_exit>Exit</_exit>
  <_howmany>How Many?</_howmany>
</rect>
    )xml"};
  gui::StringsElement strings(is);

  auto traitExit{strings.makeTrait(prefix + "_exit")};
  REQUIRE(traitExit.invoke() == "Exit");

  auto traitHowMany{strings.makeTrait(prefix + "_howmany")};
  REQUIRE(traitHowMany.invoke() == "How Many?");
}

TEST_CASE("StringsElement accepts documents without any strings", "[gui]") {
  [[maybe_unused]] auto logger{getLogger()};
  const std::string prefix{gui::StringsElement::getPrefix()};

  std::istringstream is{R"xml(
<rect name="Strings">
</rect>
    )xml"};
  gui::StringsElement strings(is);

  auto traitEmpty{strings.makeTrait(prefix + "_empty")};
  REQUIRE(traitEmpty.invoke().empty());
}

TEST_CASE("StringsElement ignores irrelevant nodes", "[gui]") {
  [[maybe_unused]] auto logger{getLogger()};
  const std::string prefix{gui::StringsElement::getPrefix()};

  std::istringstream is{R"xml(
<class name="Strings">
  <_test>First</_test>
  <_ignored>Ignored</_ignored>
</class>

<rect name="Strings">
  <_test>Second</_test>
</rect>
    )xml"};
  gui::StringsElement strings(is);

  auto traitTest{strings.makeTrait(prefix + "_test")};
  REQUIRE(traitTest.invoke() == "Second");

  auto traitIgnored{strings.makeTrait(prefix + "_ignored")};
  REQUIRE(traitIgnored.invoke().empty());
}

TEST_CASE("can use trait selectors", "[gui]") {
  std::istringstream is{R"xml(
<menu name="Example">
  <rect name="foo">
    <user1>Hello</user1>
    <x>0</x>
    <y>30</y>
  </rect>

  <rect name="bar">
    <x>10</x>
  </rect>

  <rect name="baz">
    <rect name="qux"></rect>
  </rect>
</menu>
  )xml"};

  pugi::xml_document doc;
  auto result{doc.load(is)};
  REQUIRE(result);

  SECTION("can fully qualify names", "[gui]") {
    {
      const auto set{doc.select_nodes("/menu")};
      REQUIRE_FALSE(set.empty());
      const auto node{set.first().node()};
      REQUIRE(gui::fullyQualifyName(node) == "Example");
    }

    {
      const auto set{doc.select_nodes("/menu/rect[1]")};
      REQUIRE_FALSE(set.empty());
      const auto node{set.first().node()};
      REQUIRE(gui::fullyQualifyName(node) == "Example.foo");
    }

    {
      const auto set{doc.select_nodes("/menu/rect[2]")};
      REQUIRE_FALSE(set.empty());
      const auto node{set.first().node()};
      REQUIRE(gui::fullyQualifyName(node) == "Example.bar");
    }

    {
      const auto set{doc.select_nodes("/menu/rect[1]/x")};
      REQUIRE_FALSE(set.empty());
      const auto node{set.first().node()};
      REQUIRE(gui::fullyQualifyName(node).empty());
    }
  }

  SECTION("can use the child() selector", "[gui]") {
    REQUIRE(gui::invokeChildSelector(doc, {}) == "Example");
    REQUIRE(gui::invokeChildSelector(doc, "bar") == "Example.bar");
    REQUIRE(gui::invokeChildSelector(doc, "foo") == "Example.foo");
    REQUIRE(gui::invokeChildSelector(doc.first_child(), {}) == "Example.baz");
  }

  SECTION("can use the me() selector", "[gui]") {
    const auto rectNode{doc.first_child()};
    const auto set{doc.select_nodes("/menu/rect[2]")};
    REQUIRE_FALSE(set.empty());
    const auto barNode{set.first().node()};

    REQUIRE(gui::invokeMeSelector(rectNode) == "Example");
    REQUIRE(gui::invokeMeSelector(barNode) == "Example.bar");
  }

  SECTION("can use the parent() selector", "[gui]") {
    const auto set{doc.select_nodes("/menu/rect[2]")};
    REQUIRE_FALSE(set.empty());
    const auto barNode{set.first().node()};

    REQUIRE(gui::invokeParentSelector(doc).empty());
    REQUIRE(gui::invokeParentSelector(doc.first_child()).empty());
    REQUIRE(gui::invokeParentSelector(barNode) == "Example");
    // Unintended use, but allowed under the documentation.
    REQUIRE(gui::invokeParentSelector(barNode.first_child()) == "Example.bar");
  }

  SECTION("can use the screen() selector", "[gui]") {
    REQUIRE(gui::invokeScreenSelector() == gui::ScreenElement::getName());
  }

  SECTION("can use the strings() selector", "[gui]") {
    REQUIRE(gui::invokeStringsSelector() == gui::StringsElement::getName());
  }

  SECTION("can use the sibling() selector", "[gui]") {
    const auto fooSet{doc.select_nodes("/menu/rect[1]")};
    REQUIRE_FALSE(fooSet.empty());
    const auto fooNode{fooSet.first().node()};

    const auto barSet{doc.select_nodes("/menu/rect[2]")};
    REQUIRE_FALSE(barSet.empty());
    const auto barNode{barSet.first().node()};

    const auto quxSet{doc.select_nodes("/menu/rect[3]/rect[1]")};
    REQUIRE_FALSE(quxSet.empty());
    const auto quxNode{quxSet.first().node()};

    REQUIRE(gui::invokeSiblingSelector(barNode, {}) == "Example.foo");
    REQUIRE(gui::invokeSiblingSelector(fooNode, {}).empty());
    REQUIRE(gui::invokeSiblingSelector(barNode, "foo") == "Example.foo");
    REQUIRE(gui::invokeSiblingSelector(barNode, "bar").empty());
    REQUIRE(gui::invokeSiblingSelector(quxNode, {}).empty());
  }
}