#ifndef OPENOBLIVION_GUI_INTERACTABLE_MIXIN_HPP
#define OPENOBLIVION_GUI_INTERACTABLE_MIXIN_HPP

#include "gui/ui_element.hpp"

namespace gui {

class InteractableMixin : public virtual UiElement {
 private:
  /// Whether this uiElement receives user input events
  bool mIsTarget{false};
  /// Id of this element for user input handling
  int mId{-1};

 public:

  void set_target(bool isTarget) override;
  void set_id(float id) override;

  // Note: Cannot check mIsTarget && mId >= -1 on construction because these
  // values are not set until the first update(), which must occur after all the
  // traits have been added. Thus the traits must always be added, and their
  // behaviour must depend on the condition.

  std::optional<gui::Trait<float>> make_clicked() const override;
  std::optional<gui::Trait<float>> make_shiftclicked() const override;
  std::optional<gui::Trait<float>> make_mouseover() const override;
};

} // namespace gui

#endif // OPENOBLIVION_GUI_INTERACTABLE_MIXIN_HPP
