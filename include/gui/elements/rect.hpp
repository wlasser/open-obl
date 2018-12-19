#ifndef OPENOBLIVION_GUI_RECT_HPP
#define OPENOBLIVION_GUI_RECT_HPP

#include "gui/ui_element.hpp"
#include <OgrePanelOverlayElement.h>
#include <string>

namespace gui {

class Rect : public UiElement {
 private:
  Ogre::PanelOverlayElement *mOverlay{};

 public:
  Rect(std::string name);

  ~Rect();

  void set_x(float x) override;
  void set_y(float y) override;
  void set_width(float width) override;
  void set_height(float height) override;
  void set_visible(bool visible) override;

  Ogre::OverlayElement *getOverlayElement() override {
    return mOverlay;
  }
};

} // namespace gui

#endif // OPENOBLIVION_GUI_RECT_HPP
