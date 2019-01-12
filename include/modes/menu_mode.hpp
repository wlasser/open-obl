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
/// \ingroup OpenOblivionModes
template<gui::MenuType Type> class MenuMode;

} // namespace oo

#endif // OPENOBLIVION_MENU_MODE_HPP
