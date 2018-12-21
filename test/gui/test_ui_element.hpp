#ifndef OPENOBLIVION_TEST_UI_ELEMENT_HPP
#define OPENOBLIVION_TEST_UI_ELEMENT_HPP

#include "gui/trait.hpp"
#include "gui/ui_element.hpp"
#include <variant>

namespace gui {

class TestUiElement : public gui::UiElement {
 private:
  float mWidth{};
  float mHeight{};
  float mScale{1};

 public:
  void set_width(float w) override {
    mWidth = w;
  }

  void set_height(float h) override {
    mHeight = h;
  }

  int getArea() {
    return mWidth * mHeight * mScale;
  }

  gui::TraitTypeId userTraitType(int index) const override {
    return index == 0 ? gui::TraitTypeId::Float
                      : gui::TraitTypeId::Unimplemented;
  }

  void set_user(int index, gui::UiElement::UserValue value) override {
    if (index == 0 && std::holds_alternative<float>(value)) {
      mScale = std::get<float>(value);
    }
  }
};

}

#endif // OPENOBLIVION_TEST_UI_ELEMENT_HPP
