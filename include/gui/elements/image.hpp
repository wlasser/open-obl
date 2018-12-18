#ifndef OPENOBLIVION_GUI_IMAGE_HPP
#define OPENOBLIVION_GUI_IMAGE_HPP

#include "gui/screen.hpp"
#include "gui/ui_element.hpp"
#include <OgrePanelOverlayElement.h>
#include <OgreMaterial.h>

namespace gui {

class Image : public UiElement {
 private:
  Ogre::PanelOverlayElement *mOverlay{};
  Ogre::MaterialPtr mMatPtr{};

 public:
  Image(std::string name);

  ~Image();

  void set_x(int x) override;
  void set_y(int y) override;
  void set_width(int width) override;
  void set_height(int height) override;
  void set_filename(std::string filename) override;
  void set_zoom(int zoom) override;
  void set_visible(bool visible) override;

  Ogre::OverlayElement *getOverlayElement() override {
    return mOverlay;
  }
};

} // namespace gui

#endif // OPENOBLIVION_GUI_IMAGE_HPP
