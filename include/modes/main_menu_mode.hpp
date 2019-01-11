#ifndef OPENOBLIVION_MAIN_MENU_MODE_HPP
#define OPENOBLIVION_MAIN_MENU_MODE_HPP

#include "modes/menu_mode.hpp"
#include "modes/menu_mode_base.hpp"

namespace oo {

/// \ingroup OpenOblivionModes
using MainMenuMode = MenuMode<gui::MenuType::MainMenu>;

/// \ingroup OpenOblivionModes
template<> struct MenuModeTransition<MainMenuMode> {
  using type = ModeTransition<MenuMode<gui::MenuType::MainMenu>,
  MenuMode<gui::MenuType::LoadingMenu>>;
};

/// Specialization of `oo::MenuMode` for the Main Menu.
/// \ingroup OpenOblivionModes
template<> class MenuMode<gui::MenuType::MainMenu>
    : public MenuModeBase<MainMenuMode> {
 public:
  explicit MenuMode<gui::MenuType::MainMenu>(ApplicationContext &ctx)
      : MenuModeBase<MainMenuMode>(ctx) {}

  std::string getFilenameImpl() const {
    return "menus/options/main_menu.xml";
  }

  MainMenuMode::transition_t
  handleEventImpl(ApplicationContext &ctx, const sdl::Event &event);

  void updateImpl(ApplicationContext &ctx, float delta);
};

} // namespace oo

#endif // OPENOBLIVION_MAIN_MENU_MODE_HPP
