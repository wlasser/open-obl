#ifndef OPENOBLIVION_TEST_UI_ELEMENT_HPP
#define OPENOBLIVION_TEST_UI_ELEMENT_HPP

#include "gui/trait.hpp"
#include "gui/ui_element.hpp"
#include <variant>

namespace gui {

class TestUiElement : public gui::UiElement {
 private:
  int mWidth{};
  int mHeight{};
  int mScale{1};

 public:
  void set_width(int w) override {
    mWidth = w;
  }

  void set_height(int h) override {
    mHeight = h;
  }

  int getArea() {
    return mWidth * mHeight * mScale;
  }

  gui::TraitTypeId userTraitType(int index) const override {
    return index == 0 ? gui::TraitTypeId::Int : gui::TraitTypeId::Unimplemented;
  }

  void set_user(int index, gui::UiElement::UserValue value) override {
    if (index == 0 && std::holds_alternative<int>(value)) {
      mScale = std::get<int>(value);
    }
  }
};

}

#endif // OPENOBLIVION_TEST_UI_ELEMENT_HPP
