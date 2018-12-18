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

  void set_x(int x) override;
  void set_y(int y) override;
  void set_width(int width) override;
  void set_height(int height) override;
  void set_visible(bool visible) override;

  Ogre::OverlayElement *getOverlayElement() override {
    return mOverlay;
  }
};

} // namespace gui

#endif // OPENOBLIVION_GUI_RECT_HPP
