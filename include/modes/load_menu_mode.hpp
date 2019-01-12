#ifndef OPENOBLIVION_LOAD_MENU_MODE_HPP
#define OPENOBLIVION_LOAD_MENU_MODE_HPP

#include "modes/menu_mode.hpp"
#include "modes/menu_mode_base.hpp"

namespace oo {

class GameMode;

/// \ingroup OpenOblivionModes
template<> struct MenuModeTransition<LoadMenuMode> {
  using type = ModeTransition<LoadMenuMode, GameMode>;
};

/// Specialization of `oo::MenuMode` for the Load Game Menu.
/// \ingroup OpenOblivionModes
template<> class MenuMode<gui::MenuType::LoadMenu>
    : public MenuModeBase<LoadMenuMode> {
 public:
  explicit MenuMode<gui::MenuType::LoadMenu>(ApplicationContext &ctx)
      : MenuModeBase<LoadMenuMode>(ctx) {}

  std::string getFilenameImpl() const {
    return "menus/options/load_menu.xml";
  }

  LoadMenuMode::transition_t
  handleEventImpl(ApplicationContext &ctx, const sdl::Event &event);

  void updateImpl(ApplicationContext &ctx, float delta);
};

} // namespace oo

#endif // OPENOBLIVION_LOAD_MENU_MODE_HPP
