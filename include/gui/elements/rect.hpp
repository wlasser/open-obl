#ifndef OPENOBLIVION_GUI_RECT_HPP
#define OPENOBLIVION_GUI_RECT_HPP

#include "gui/ui_element.hpp"

namespace gui {

class Rect : public UiElement {
 public:
  Rect(std::string name) {
    set_name(std::move(name));
  }
};

} // namespace gui

#endif // OPENOBLIVION_GUI_RECT_HPP
