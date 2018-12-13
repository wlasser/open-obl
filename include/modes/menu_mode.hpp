#ifndef OPENOBLIVION_MENU_MODE_HPP
#define OPENOBLIVION_MENU_MODE_HPP

#include "application_context.hpp"
#include "gui/gui.hpp"
#include "gui/menu.hpp"
#include "modes/mode.hpp"
#include "sdl/sdl.hpp"

class ConsoleMode;
class GameMode;

class MenuMode {
 private:
  std::optional<gui::MenuContext> mMenuCtx{};

  std::optional<gui::MenuContext> loadMenu(gui::MenuType type);

 public:
  using transition_t = ModeTransition<MenuMode>;

  explicit MenuMode(ApplicationContext &ctx, gui::MenuType type);

  void enter(ApplicationContext &ctx) {
    refocus(ctx);
  }

  void refocus(ApplicationContext &) {
    sdl::setRelativeMouseMode(false);
  }

  transition_t handleEvent(ApplicationContext &ctx, const sdl::Event &event);

  void update(ApplicationContext &ctx, float delta);
};

#endif // OPENOBLIVION_MENU_MODE_HPP
