#ifndef OPENOBLIVION_MAIN_MENU_MODE_HPP
#define OPENOBLIVION_MAIN_MENU_MODE_HPP

#include "modes/menu_mode.hpp"
#include "modes/menu_mode_base.hpp"

namespace oo {

using MainMenuMode = MenuMode<gui::MenuType::MainMenu>;

template<> struct MenuModeTransition<MainMenuMode> {
  using type = ModeTransition<MainMenuMode, MenuMode<gui::MenuType::LoadingMenu>>;
};

template<>
class MenuMode<gui::MenuType::MainMenu> : public MenuModeBase<MainMenuMode> {
 public:
  using transition_t = MenuModeTransition_t<MainMenuMode>;

  explicit MenuMode<gui::MenuType::MainMenu>(ApplicationContext &ctx)
      : MenuModeBase<MainMenuMode>(ctx) {}

  std::string getFilenameImpl() const {
    return "menus/options/main_menu.xml";
  }

  transition_t handleEventImpl(ApplicationContext &ctx,
                               const sdl::Event &event);
  void updateImpl(ApplicationContext &ctx, float delta);
};

} // namespace oo

#endif // OPENOBLIVION_MAIN_MENU_MODE_HPP
