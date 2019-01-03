#ifndef OPENOBLIVION_UI_PANEL_MIXIN_HPP
#define OPENOBLIVION_UI_PANEL_MIXIN_HPP

#include "gui/ui_element.hpp"
#include <OgrePanelOverlayElement.h>

namespace gui {

class PanelMixin : public virtual UiElement {
 private:
  Ogre::PanelOverlayElement *mOverlay{};

 protected:
  Ogre::PanelOverlayElement *getPanelOverlayElement();

 public:
  PanelMixin(const std::string &name);

  ~PanelMixin();

  void set_x(float x) override;
  void set_y(float y) override;
  void set_width(float width) override;
  void set_height(float height) override;
  void set_visible(bool visible) override;

  Ogre::OverlayElement *getOverlayElement() override;
};

} // namespace gui

#endif // OPENOBLIVION_UI_PANEL_MIXIN_HPP
