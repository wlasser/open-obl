#ifndef OPENOBLIVION_GUI_INTERACTABLE_MIXIN_HPP
#define OPENOBLIVION_GUI_INTERACTABLE_MIXIN_HPP

#include "gui/ui_element.hpp"

namespace gui {

class InteractableMixin : public virtual UiElement {
 private:
  /// Id of this element for user input handling
  int mId{-1};
  /// Whether this uiElement receives user input events
  bool mIsTarget{false};
  /// Whether this uiElement has been clicked this frame
  bool mIsClicked{false};
  /// Whether this uiElement has been shift-clicked this frame
  bool mIsShiftclicked{false};
  /// Whether the mouse cursor is over this element during this frame
  bool mIsMouseover{false};

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

  void clearEvents() override;

  void notify_clicked() override;
  void notify_shiftclicked() override;
  void notify_mouseover() override;

  bool is_clicked() const override;
  bool is_shiftclicked() const override;
  bool is_mouseover() const override;
};

} // namespace gui

#endif // OPENOBLIVION_GUI_INTERACTABLE_MIXIN_HPP
