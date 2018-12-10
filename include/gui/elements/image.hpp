#ifndef OPENOBLIVION_GUI_IMAGE_HPP
#define OPENOBLIVION_GUI_IMAGE_HPP

#include "gui/screen.hpp"
#include "gui/ui_element.hpp"
#include <OgrePanelOverlayElement.h>

namespace gui {

class Image : public UiElement {
 private:
  Ogre::PanelOverlayElement *mOverlay{};

 public:
  Image(std::string name);

  void set_x(int x) override;
  void set_y(int y) override;
  void set_width(int width) override;
  void set_height(int height) override;
  void set_filename(std::string filename) override;
  void set_zoom(int zoom) override;

  Ogre::OverlayElement *getOverlayElement() const override {
    return mOverlay;
  }
};

} // namespace gui

#endif // OPENOBLIVION_GUI_IMAGE_HPP
