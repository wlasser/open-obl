#include "gui/elements/interactable_mixin.hpp"

void gui::InteractableMixin::set_target(bool isTarget) {
  mIsTarget = isTarget;
}

void gui::InteractableMixin::set_id(float id) {
  mId = static_cast<int>(id);
}

std::optional<gui::Trait<float>>
gui::InteractableMixin::make_clicked() const {
  if (!getOverlayElement()) {
    return gui::Trait<float>(get_name() + ".clicked", 0.0f);
  }

  gui::TraitFun<float> fun{[this]() -> float {
    return this->is_clicked() ? 1.0f : 0.0f;
  }};
  return gui::Trait<float>(get_name() + ".clicked", std::move(fun));
}

std::optional<gui::Trait<float>>
gui::InteractableMixin::make_shiftclicked() const {
  if (!getOverlayElement()) {
    return gui::Trait<float>(get_name() + ".shiftclicked", 0.0f);
  }

  gui::TraitFun<float> fun{[this]() -> float {
    return this->is_shiftclicked() ? 1.0f : 0.0f;
  }};
  return gui::Trait<float>(get_name() + ".shiftclicked", std::move(fun));
}

std::optional<gui::Trait<float>>
gui::InteractableMixin::make_mouseover() const {
  if (!getOverlayElement()) {
    return gui::Trait<float>(get_name() + ".mouseover", 0.0f);
  }

  gui::TraitFun<float> fun{[this]() -> float {
    return this->is_mouseover() ? 1.0f : 0.0f;
  }};
  return gui::Trait<float>(get_name() + ".mouseover", std::move(fun));
}

void gui::InteractableMixin::clearEvents() {
  mIsClicked = false;
  mIsShiftclicked = false;
  mIsMouseover = false;
}

void gui::InteractableMixin::notify_clicked() {
  mIsClicked = true;
}

void gui::InteractableMixin::notify_shiftclicked() {
  mIsShiftclicked = true;
}

void gui::InteractableMixin::notify_mouseover() {
  mIsMouseover = true;
}

bool gui::InteractableMixin::is_clicked() const {
  return mIsClicked;
}

bool gui::InteractableMixin::is_shiftclicked() const {
  return mIsShiftclicked;
}

bool gui::InteractableMixin::is_mouseover() const {
  return mIsMouseover;
}
