#ifndef OPENOBLIVION_MENU_MODE_BASE_HPP
#define OPENOBLIVION_MENU_MODE_BASE_HPP

#include "application_context.hpp"
#include "gui/gui.hpp"
#include "gui/menu.hpp"
#include "modes/mode.hpp"
#include "sdl/sdl.hpp"
#include <OgreOverlayElement.h>
#include <OgreOverlayContainer.h>

namespace oo {

template<gui::MenuType Type> class MenuMode;

template<class T> struct MenuModeTransition;

template<class T>
using MenuModeTransition_t = typename MenuModeTransition<T>::type;

template<class Self>
class MenuModeBase {
 private:
  std::optional<gui::MenuContext> mMenuCtx{};
  float mClock{0.0f};
  Ogre::Vector2 mCursorPos{};

  Self &self() {
    return static_cast<Self &>(*this);
  }

  const Self &self() const {
    return static_cast<const Self &>(*this);
  }

 protected:
  /// Find the `gui::UiElement` under the cursor and call `f` on it and its
  /// ancestors in decreasing order of generation.
  template<class F> void notifyElementAtCursor(F &&f);

 public:
  explicit MenuModeBase(ApplicationContext &ctx);

  void enter(ApplicationContext &ctx) {
    refocus(ctx);
  }

  void refocus(ApplicationContext &) {
    sdl::setRelativeMouseMode(false);
  }

  MenuModeTransition_t<Self>
  handleEvent(ApplicationContext &ctx, const sdl::Event &event);

  void update(ApplicationContext &ctx, float delta);

  std::string getFilename() const {
    return self().getFilenameImpl();
  }

  gui::MenuContext *getMenuCtx() {
    return &*mMenuCtx;
  }

  float getClock() {
    return mClock;
  }
};

template<class Self> template<class F> void
MenuModeBase<Self>::notifyElementAtCursor(F &&f) {
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

template<class Self> MenuModeBase<Self>::MenuModeBase(ApplicationContext &ctx) {
  refocus(ctx);
  mMenuCtx = gui::loadMenu(getFilename(), "menus/strings.xml");
  if (!mMenuCtx) {
    throw std::runtime_error("Failed to construct menu");
  }
  mMenuCtx->update();
}

template<class Self> MenuModeTransition_t<Self>
MenuModeBase<Self>::handleEvent(ApplicationContext &ctx,
                                const sdl::Event &event) {
  // Pop if a event::MenuMode is pressed
  if (auto keyEvent{ctx.getKeyMap().translateKey(event)}) {
    if (auto *ev{std::get_if<oo::event::MenuMode>(&*keyEvent)}; ev) {
      return {ev->down, std::nullopt};
    }
  } else if (sdl::typeOf(event) == sdl::EventType::MouseMotion) {
    if (!mMenuCtx) return {false, std::nullopt};
    mCursorPos = mMenuCtx->normalizeCoordinates(event.motion.x, event.motion.y);
  } else if (sdl::typeOf(event) == sdl::EventType::MouseButtonDown) {
    if (sdl::mouseButtonOf(event.button) != sdl::MouseButton::Left) {
      return {false, std::nullopt};
    }
    if (!mMenuCtx) return {false, std::nullopt};
    notifyElementAtCursor([](gui::UiElement *elem) {
      if (!!(sdl::getModState() & sdl::ModifierKey::Shift)) {
        elem->notify_shiftclicked();
      } else {
        elem->notify_clicked();
      }
    });
  }

  return self().handleEventImpl(ctx, event);
}

template<class Self> void
MenuModeBase<Self>::update(ApplicationContext &ctx, float delta) {
  // Mouseover events are triggered every frame, not just on mouse move.
  notifyElementAtCursor([](gui::UiElement *elem) { elem->notify_mouseover(); });

  mClock += delta;

  self().updateImpl(ctx, delta);
  mMenuCtx->update();
  mMenuCtx->clearEvents();
}

} // namespace oo

#endif // OPENOBLIVION_MENU_MODE_BASE_HPP
