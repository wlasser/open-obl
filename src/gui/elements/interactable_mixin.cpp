#include "gui/elements/interactable_mixin.hpp"

void gui::InteractableMixin::set_target(bool isTarget) {
  mIsTarget = isTarget;
}

void gui::InteractableMixin::set_id(float id) {
  mId = static_cast<int>(id);
}

std::optional<gui::Trait<float>>
gui::InteractableMixin::make_clicked() const {
  gui::TraitFun<float> fun{[]() -> float { return 0.0f; }};
  return gui::Trait<float>(get_name() + ".clicked", std::move(fun));
}

std::optional<gui::Trait<float>>
gui::InteractableMixin::make_shiftclicked() const {
  gui::TraitFun<float> fun{[]() -> float { return 0.0f; }};
  return gui::Trait<float>(get_name() + ".shiftclicked", std::move(fun));
}

std::optional<gui::Trait<float>>
gui::InteractableMixin::make_mouseover() const {
  gui::TraitFun<float> fun{[]() -> float { return 0.0f; }};
  return gui::Trait<float>(get_name() + ".mouseover", std::move(fun));
}
