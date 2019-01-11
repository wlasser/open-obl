#ifndef OPENOBLIVION_MENU_MODE_HPP
#define OPENOBLIVION_MENU_MODE_HPP

#include "modes/menu_mode_base.hpp"

namespace oo {

/// \ingroup OpenOblivionModes
template<gui::MenuType Type> struct MenuModeTransition<MenuMode<Type>> {
  using type = ModeTransition<MenuMode<Type>>;
};

/// Class template representing a generic menu `Mode`, should be specialized
/// for each value of `gui::MenuType`.
/// Simply provides empty implementations of the methods required by
/// `oo::MenuModeBase`.
/// \ingroup OpenOblivionModes
template<gui::MenuType Type>
class MenuMode : public MenuModeBase<MenuMode<Type>> {
 public:
  explicit MenuMode(ApplicationContext &ctx)
      : MenuModeBase<MenuMode<Type>>(ctx) {}

  std::string getFilenameImpl() const {
    return "";
  }

  typename MenuMode::transition_t
  handleEventImpl(ApplicationContext &/*ctx*/, const sdl::Event &/*event*/) {
    return {false, std::nullopt};
  }

  void updateImpl(ApplicationContext &/*ctx*/, float /*delta*/) {}
};

} // namespace oo

#endif // OPENOBLIVION_MENU_MODE_HPP
