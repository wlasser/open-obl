#ifndef OPENOBL_MENU_MODE_HPP
#define OPENOBL_MENU_MODE_HPP

#include "modes/menu_mode_base.hpp"

namespace oo {

/// \ingroup OpenOBLModes
template<gui::MenuType Type> struct MenuModeTransition<MenuMode<Type>> {
  using type = ModeTransition<MenuMode<Type>>;
};

/// \name Menu mode short names
/// \addtogroup OpenOBLModes
/// @{
using LoadMenuMode = MenuMode<gui::MenuType::LoadMenu>;
using LoadingMenuMode = MenuMode<gui::MenuType::LoadingMenu>;
using MainMenuMode = MenuMode<gui::MenuType::MainMenu>;
/// @}

/// Class template representing a generic menu `Mode`, should be specialized
/// for each value of `gui::MenuType`.
/// \ingroup OpenOBLModes
template<gui::MenuType Type> class MenuMode;

} // namespace oo

#endif // OPENOBL_MENU_MODE_HPP
