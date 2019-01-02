#ifndef OPENOBLIVION_GUI_RECT_HPP
#define OPENOBLIVION_GUI_RECT_HPP

#include "gui/ui_element.hpp"
#include <OgrePanelOverlayElement.h>
#include <string>

namespace gui {

class Rect : public UiElement {
 private:
  Ogre::PanelOverlayElement *mOverlay{};

  /// Whether this Rect receives user input events
  bool mIsTarget{false};
  /// Id of this element for user input handling
  int mId{-1};

 public:
  Rect(std::string name);

  ~Rect();

  void set_x(float x) override;
  void set_y(float y) override;
  void set_width(float width) override;
  void set_height(float height) override;
  void set_visible(bool visible) override;
  void set_target(bool isTarget) override;
  void set_id(float id) override;

  std::optional<gui::Trait<float>> make_clicked() const override;
  std::optional<gui::Trait<float>> make_shiftclicked() const override;
  std::optional<gui::Trait<float>> make_mouseover() const override;

  Ogre::OverlayElement *getOverlayElement() override {
    return mOverlay;
  }
};

} // namespace gui

#endif // OPENOBLIVION_GUI_RECT_HPP
