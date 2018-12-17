#include "gui/screen.hpp"
#include "gui/strings.hpp"
#include "gui/trait.hpp"
#include "gui/traits.hpp"
#include "gui/trait_selector.hpp"
#include "settings.hpp"
#include "test_ui_element.hpp"
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
  pugi::xml_document doc{};
  REQUIRE(doc.load(is));
  gui::StringsElement strings(doc);

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
  pugi::xml_document doc{};
  REQUIRE(doc.load(is));
  gui::StringsElement strings(doc);

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
  pugi::xml_document doc{};
  REQUIRE(doc.load(is));
  gui::StringsElement strings(doc);

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
    <y>
      <copy src="bar" trait="x"/>
    </y>
  </rect>

  <rect name="bar">
    <x>10</x>
    <y>
      <copy src="me()" trait="x"/>
    </y>
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
      const auto node{doc.select_node("/menu").node()};
      REQUIRE(node);
      REQUIRE(gui::fullyQualifyName(node) == "Example");
    }

    {
      const auto node{doc.select_node("/menu/rect[1]").node()};
      REQUIRE(node);
      REQUIRE(gui::fullyQualifyName(node) == "Example.foo");
    }

    {
      const auto node{doc.select_node("/menu/rect[2]").node()};
      REQUIRE(node);
      REQUIRE(gui::fullyQualifyName(node) == "Example.bar");
    }

    {
      const auto node{doc.select_node("/menu/rect[1]/x").node()};
      REQUIRE(node);
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
    REQUIRE(rectNode);
    REQUIRE(gui::invokeMeSelector(rectNode) == "Example");

    const auto barNode{doc.select_node("/menu/rect[2]").node()};
    REQUIRE(barNode);
    REQUIRE(gui::invokeMeSelector(barNode) == "Example.bar");

    const auto copyNode{doc.select_node("/menu/rect[2]/y/copy").node()};
    REQUIRE(copyNode);
    REQUIRE(gui::resolveTrait(copyNode) == "Example.bar.x");
  }

  SECTION("can use the parent() selector", "[gui]") {
    const auto barNode{doc.select_node("/menu/rect[2]").node()};
    REQUIRE(barNode);

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
    const auto fooNode{doc.select_node("/menu/rect[1]").node()};
    REQUIRE(fooNode);

    const auto barNode{doc.select_node("/menu/rect[2]").node()};
    REQUIRE(barNode);

    const auto quxNode{doc.select_node("/menu/rect[3]/rect[1]").node()};
    REQUIRE(quxNode);

    REQUIRE(gui::invokeSiblingSelector(barNode, {}) == "Example.foo");
    REQUIRE(gui::invokeSiblingSelector(fooNode, {}).empty());
    REQUIRE(gui::invokeSiblingSelector(barNode, "foo") == "Example.foo");
    REQUIRE(gui::invokeSiblingSelector(barNode, "bar").empty());
    REQUIRE(gui::invokeSiblingSelector(quxNode, {}).empty());
  }

  SECTION("can resolve src/trait pairs") {
    const auto copyNode{doc.select_node("/menu/rect[1]/y/copy").node()};
    REQUIRE(copyNode);
    REQUIRE(gui::resolveTrait(copyNode) == "Example.bar.x");
  }
}

TEST_CASE("can add traits without dependencies to Traits", "[gui]") {
  gui::Traits traits{};

  {
    auto &trait{traits.addTrait<int>("t1", 10)};
    REQUIRE(trait.invoke() == 10);
    REQUIRE(trait.getName() == "t1");
  }

  {
    const auto &trait{traits.getTrait<int>("t1")};
    REQUIRE(trait.invoke() == 10);
    REQUIRE(trait.getName() == "t1");
    REQUIRE_THROWS_AS(traits.getTrait<float>("t1"), std::runtime_error);
    REQUIRE_THROWS_AS(traits.getTrait<int>("t2"), std::runtime_error);
  }

  {
    gui::Trait<std::string> trait("t2", "Hello");
    auto &traitRef{traits.addTrait(std::move(trait))};
    REQUIRE(traitRef.invoke() == "Hello");
    REQUIRE(traitRef.getName() == "t2");
  }
}

TEST_CASE("can add traits with dependencies to Traits", "[gui]") {
  gui::Traits traits{};

  // t1() = t1Src;
  // t2() = 3 * t1();
  // t3() = hexadecimal string representation of t2()
  int t1Src{10};
  gui::TraitFun<int> t1Fun{[&t1Src]() { return t1Src; }};
  auto &t1{traits.addTrait<int>("t1", std::move(t1Fun))};

  gui::TraitFun<int> t2Fun{[&traits]() -> int {
    return 3 * traits.getTrait<int>("t1").invoke();
  }};

  auto &t2{traits.addTrait<int>("t2", std::move(t2Fun))};
  REQUIRE(t2.invoke() == 30);

  gui::TraitFun<std::string> t3Fun{[&traits]() -> std::string {
    std::ostringstream os;
    os << std::hex << traits.getTrait<int>("t2").invoke();
    return os.str();
  }};

  auto &t3{traits.addTrait<std::string>("t3", std::move(t3Fun))};

  t1Src = 5;
  REQUIRE(t1.invoke() == 5);
  REQUIRE(t2.invoke() == 15);
  REQUIRE(t3.invoke() == "f");
}

TEST_CASE("can bind traits to UiElements using Traits", "[gui]") {
  // The scenario here is somewhat artificial because a trait depends on the
  // uiElement directly, instead of going through a user trait interface.
  gui::TestUiElement uiElement{};
  uiElement.set_name("test");

  gui::Traits traits{};

  // t1 is an 'output' depending on the visible state of uiElement. This is for
  // dependency checking, it doesn't mirror normal usage. Note that t1 does not
  // have any implicit dependencies, since it calls getArea() directly instead
  // of invoking a trait.
  gui::TraitFun<int> t1Fun([&uiElement]() -> int {
    return uiElement.getArea();
  });
  t1Fun.addDependency("test.width");
  t1Fun.addDependency("test.height");
  t1Fun.addDependency("test.user0");
  auto &t1{traits.addTrait<int>("t1", std::move(t1Fun))};

  int width{10};
  gui::TraitFun<int> widthFun{[&width]() -> int { return width; }};
  auto &tWidth{traits.addTrait<int>("test.width", std::move(widthFun))};
  tWidth.bind(&uiElement, &gui::UiElement::set_width);

  auto &tHeight{traits.addTrait<int>("test.height", 10)};
  tHeight.bind(&uiElement, &gui::UiElement::set_height);

  int user0{1};
  gui::TraitFun<int> user0Fun{[&user0]() -> int { return user0; }};
  auto &tUser0{traits.addTrait<int>("test.user0", std::move(user0Fun))};
  tUser0.bind(&uiElement, [](gui::UiElement *uiElement, int value) {
    uiElement->set_user(0, value);
  });

  traits.addTraitDependencies();

  traits.update();
  REQUIRE(t1.invoke() == 100);

  width = 5;
  REQUIRE(t1.invoke() == 100);
  traits.update();
  REQUIRE(t1.invoke() == 50);

  user0 = 10;
  width = 2;
  REQUIRE(t1.invoke() == 50);
  traits.update();
  REQUIRE(t1.invoke() == 200);
}

TEST_CASE("can create traits from XML and bind them", "[gui]") {
  std::istringstream is{R"xml(
<rect name="test">
  <width>10</width>
  <height>5</height>
  <user0>1</user0>
</rect>
    )xml"};

  pugi::xml_document doc;
  REQUIRE(doc.load(is));
  auto rectNode{doc.first_child()};
  auto widthNode{rectNode.first_child()};
  auto heightNode{widthNode.next_sibling()};
  auto user0Node{heightNode.next_sibling()};

  {
    gui::TestUiElement uiElement{};
    uiElement.set_name("test");

    gui::Traits traits{};

    REQUIRE_FALSE(traits.addAndBindUserTrait(widthNode, &uiElement));
    REQUIRE(traits.addAndBindImplementationTrait(widthNode, &uiElement));
    REQUIRE(traits.addAndBindImplementationTrait(heightNode, &uiElement));
    REQUIRE(traits.addAndBindUserTrait(user0Node, &uiElement));
    traits.addTraitDependencies();
    traits.update();
    REQUIRE(uiElement.getArea() == 50);
  }
}