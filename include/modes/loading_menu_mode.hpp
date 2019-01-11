#ifndef OPENOBLIVION_LOADING_MENU_MODE_HPP
#define OPENOBLIVION_LOADING_MENU_MODE_HPP

#include "modes/menu_mode.hpp"
#include "modes/menu_mode_base.hpp"

namespace oo {

using LoadingMenuMode = MenuMode<gui::MenuType::LoadingMenu>;

template<> struct MenuModeTransition<LoadingMenuMode> {
  using type = ModeTransition<LoadingMenuMode>;
};

template<>
class MenuMode<gui::MenuType::LoadingMenu>
    : public MenuModeBase<LoadingMenuMode> {
 public:
  using transition_t = MenuModeTransition_t<LoadingMenuMode>;

  explicit MenuMode<gui::MenuType::LoadingMenu>(ApplicationContext &ctx)
      : MenuModeBase<LoadingMenuMode>(ctx) {}

  std::string getFilenameImpl() const {
    return "menus/loading_menu.xml";
  }

  transition_t handleEventImpl(ApplicationContext &ctx,
                               const sdl::Event &event);
  void updateImpl(ApplicationContext &ctx, float delta);
};

} // namespace oo

#endif // OPENOBLIVION_LOADING_MENU_MODE_HPP
