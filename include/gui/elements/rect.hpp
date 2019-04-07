#ifndef OPENOBLIVION_GUI_RECT_HPP
#define OPENOBLIVION_GUI_RECT_HPP

#include "gui/elements/interactable_mixin.hpp"
#include "gui/elements/panel_mixin.hpp"
#include "gui/ui_element.hpp"
#include <OgrePanelOverlayElement.h>
#include <string>

namespace gui {

class Rect : public InteractableMixin, public PanelMixin {
 private:
  std::string mString;

 public:
  explicit Rect(std::string name);

  /// A common idiom is to use the string trait as a parameter to set the text
  /// in prefab buttons.
  void set_string(std::string str) override;
};

} // namespace gui

#endif // OPENOBLIVION_GUI_RECT_HPP
