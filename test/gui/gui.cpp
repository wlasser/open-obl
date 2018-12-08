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
