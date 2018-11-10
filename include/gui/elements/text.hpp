#ifndef OPENOBLIVION_GUI_TEXT_HPP
#define OPENOBLIVION_GUI_TEXT_HPP

#include "gui/trait.hpp"
#include "gui/ui_element.hpp"
#include <optional>

namespace gui {

class Text : public UiElement {
 public:
  std::optional<Trait < int>> make_width() const override {
    return Trait<int>(get_name() + ".width", 0);
  }

  std::optional<Trait < int>> make_height() const override {
    return Trait<int>(get_name() + ".height", 0);
  }
};

} // namespace gui

#endif // OPENOBLIVION_GUI_TEXT_HPP