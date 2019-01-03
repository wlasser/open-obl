#include "gui/elements/rect.hpp"
#include "gui/screen.hpp"
#include <OgreOverlayManager.h>

gui::Rect::Rect(std::string name) : PanelMixin(name) {
  set_name(std::move(name));
}
