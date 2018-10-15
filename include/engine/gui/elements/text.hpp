#ifndef OPENOBLIVION_ENGINE_GUI_TEXT_HPP
#define OPENOBLIVION_ENGINE_GUI_TEXT_HPP

#include "engine/gui/trait.hpp"
#include "engine/gui/ui_element.hpp"
#include <optional>

namespace engine::gui {

class Text : public UiElement {
 public:
  std::optional<Trait < int>> make_width() const override {
    return Trait<int>(get_name() + ".width", 0);
  }

  std::optional<Trait < int>> make_height() const override {
    return Trait<int>(get_name() + ".height", 0);
  }
};

} // namespace engine::gui

#endif // OPENOBLIVION_ENGINE_GUI_TEXT_HPP
