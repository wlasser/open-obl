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

  void set_x(float x) override;
  void set_y(float y) override;
  void set_width(float width) override;
  void set_height(float height) override;
  void set_filename(std::string filename) override;
  void set_zoom(float zoom) override;
  void set_visible(bool visible) override;

  Ogre::OverlayElement *getOverlayElement() override {
    return mOverlay;
  }
};

} // namespace gui

#endif // OPENOBLIVION_GUI_IMAGE_HPP
