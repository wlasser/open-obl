#ifndef OPENOBLIVION_GUI_RECT_HPP
#define OPENOBLIVION_GUI_RECT_HPP

#include "gui/elements/interactable_mixin.hpp"
#include "gui/elements/panel_mixin.hpp"
#include "gui/ui_element.hpp"
#include <OgrePanelOverlayElement.h>
#include <string>

namespace gui {

class Rect : public InteractableMixin, public PanelMixin {
 public:
  Rect(std::string name);
};

} // namespace gui

#endif // OPENOBLIVION_GUI_RECT_HPP
