#ifndef OPENOBL_UI_PANEL_MIXIN_HPP
#define OPENOBL_UI_PANEL_MIXIN_HPP

#include "gui/ui_element.hpp"
#include <OgrePanelOverlayElement.h>

namespace gui {

class PanelMixin : public virtual UiElement {
 private:
  Ogre::PanelOverlayElement *mOverlay{};

 protected:
  Ogre::PanelOverlayElement *getPanelOverlayElement();

 public:
  explicit PanelMixin(const std::string &name);

  ~PanelMixin() override;

  void set_x(float x) override;
  void set_y(float y) override;
  void set_width(float width) override;
  void set_height(float height) override;
  void set_depth(float depth) override;
  void set_visible(bool visible) override;

  Ogre::OverlayElement *getOverlayElement() const override;
};

} // namespace gui

#endif // OPENOBL_UI_PANEL_MIXIN_HPP
