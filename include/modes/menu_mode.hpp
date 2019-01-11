#ifndef OPENOBLIVION_MENU_MODE_HPP
#define OPENOBLIVION_MENU_MODE_HPP

#include "modes/menu_mode_base.hpp"

namespace oo {

template<gui::MenuType Type> struct MenuModeTransition<MenuMode<Type>> {
  using type = ModeTransition<MenuMode<Type>>;
};

template<gui::MenuType Type>
class MenuMode : public MenuModeBase<MenuMode<Type>> {
 public:
  using transition_t = MenuModeTransition_t<MenuMode<Type>>;

  explicit MenuMode(ApplicationContext &ctx)
      : MenuModeBase<MenuMode<Type>>(ctx) {}

  std::string getFilenameImpl() const {
    return "";
  }

  transition_t handleEventImpl(ApplicationContext &/*ctx*/,
                               const sdl::Event &/*event*/) {
    return {false, std::nullopt};
  }

  void updateImpl(ApplicationContext &/*ctx*/, float /*delta*/) {}
};

} // namespace oo

#endif // OPENOBLIVION_MENU_MODE_HPP
