#ifndef OPENOBLIVION_MENU_MODE_HPP
#define OPENOBLIVION_MENU_MODE_HPP

#include "application_context.hpp"
#include "gui/gui.hpp"
#include "gui/menu.hpp"
#include "modes/mode.hpp"
#include "sdl/sdl.hpp"
#include <OgreOverlayElement.h>
#include <OgreOverlayContainer.h>

namespace oo {

class ConsoleMode;
class GameMode;

class MenuMode {
 private:
  std::optional<gui::MenuContext> mMenuCtx{};
  gui::MenuType mMenuType{gui::MenuType::N};

  float mClock{0.0f};
  Ogre::Vector2 mCursorPos{};

  std::optional<gui::MenuContext> loadMenu(gui::MenuType type);

  /// Find the `gui::UiElement` under the cursor and call `f` on it and its
  /// ancestors in decreasing order of generation.
  template<class F> void notifyElementAtCursor(F &&f);

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

template<class F> void MenuMode::notifyElementAtCursor(F &&f) {
  auto *overlay{mMenuCtx->getOverlay()};
  if (!overlay) return;

  auto *overlayElement{overlay->findElementAt(mCursorPos.x, mCursorPos.y)};
  while (overlayElement) {
    const auto &any{overlayElement->getUserObjectBindings().getUserAny()};
    auto *uiElement{Ogre::any_cast<gui::UiElement *>(any)};
    f(uiElement);
    overlayElement = overlayElement->getParent();
  }
}

} // namespace oo

#endif // OPENOBLIVION_MENU_MODE_HPP
