#include "gui/gui.hpp"
#include "gui/stack/program.hpp"
#include "gui/traits.hpp"
#include "test_ui_element.hpp"
#include <catch2/catch.hpp>
#include <sstream>

TEST_CASE("can use traits in the stack machine", "[gui][gui/stack]") {
  std::istringstream is{R"xml(
<test name="foo">
  <width>10</width>
  <height>
    <copy src="foo" trait="width" />
    <add>5</add>
  </height>
  <user0>1</user0>
</test>
  )xml"};
  pugi::xml_document doc{};
  REQUIRE(doc.load(is));

  gui::TestUiElement uiElement{};
  uiElement.set_name("foo");

  gui::Traits traits{};

  auto testNode{doc.first_child()};
  auto widthNode{testNode.first_child()};
  auto heightNode{widthNode.next_sibling()};
  auto user0Node{heightNode.next_sibling()};

  traits.addAndBindImplementationTrait(widthNode, &uiElement);
  traits.addAndBindUserTrait(user0Node, &uiElement);

  SECTION("hardcoded trait function") {
    gui::stack::Program heightProg{gui::stack::compile(heightNode, &traits)};
    gui::TraitFun<int> heightFun([heightProg]() -> int {
      return std::get<int>(heightProg());
    });
    for (const auto &dep : heightProg.dependencies) {
      heightFun.addDependency(dep);
    }
    auto &heightTrait{traits.addTrait<int>("foo.height", std::move(heightFun))};
    heightTrait.bind(&uiElement, &gui::UiElement::set_height);
  }

  SECTION("using getTraitFun", "[gui][gui/stack]") {
    gui::TraitFun<int> heightFun(gui::getTraitFun<int>(traits, heightNode));
    auto &heightTrait{traits.addTrait<int>("foo.height", std::move(heightFun))};
    heightTrait.bind(&uiElement, &gui::UiElement::set_height);
  }

  SECTION("using andAndBindImplementationTrait", "[gui][gui/stack]") {
    traits.addAndBindImplementationTrait(heightNode, &uiElement);
  }

  traits.addTraitDependencies();
  traits.update();

  REQUIRE(uiElement.getArea() == 150);
}

// This behaviour is not encouraged but is required due to some awkwardly
// written gui files.
TEST_CASE("can have sibling uiElements with the same name", "[gui]") {
  std::istringstream is{R"xml(
<rect name="test">
  <rect name="dup">
    <x>10</x>
  </rect>

  <rect name="dup">
    <x>5</x>
  </rect>
</rect>
  )xml"};

  pugi::xml_document doc;
  REQUIRE(doc.load(is));
  auto testNode{doc.first_child()};
  auto dupNode1{testNode.first_child()};
  auto dupNode2{dupNode1.next_sibling()};

  auto elems{gui::getChildElements(testNode)};
  REQUIRE(elems.size() == 2);
  REQUIRE(elems[0].second == dupNode1);
  REQUIRE(elems[1].second == dupNode2);
  REQUIRE(elems[0].first->get_name() != elems[1].first->get_name());
}

TEST_CASE("user traits should not be reset to 0 on second update()",
          "[gui][regression]") {
  std::istringstream is{R"xml(
<menu name="LoadingMenu">
  <class> &LoadingMenu; </class>
  <user0> 0 </user0>
  <user1> foo.dds </user1>
  <user2> Missing </user2>
  <user3> 0 </user3>
  <user4> 100 </user4>
  <user5 />

  <image name="foo">
    <width>
      <copy src="LoadingMenu" trait="user3" />
      <div src="LoadingMenu" trait="user4" />
    </width>
  </image>
</menu>
  )xml"};
  pugi::xml_document doc{};
  REQUIRE(doc.load(is));
  auto ctx{gui::loadMenu(doc)};
  REQUIRE(ctx.has_value());
  REQUIRE_NOTHROW(ctx->update());
}