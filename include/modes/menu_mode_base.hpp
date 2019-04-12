#ifndef OPENOBLIVION_MENU_MODE_BASE_HPP
#define OPENOBLIVION_MENU_MODE_BASE_HPP

#include "application_context.hpp"
#include "controls.hpp"
#include "gui/gui.hpp"
#include "gui/menu.hpp"
#include "gui/sound.hpp"
#include "modes/mode.hpp"
#include <OgreOverlayElement.h>
#include <OgreOverlayContainer.h>

namespace oo {

template<gui::MenuType Type> class MenuMode;

/// Trait providing the `transition_t` type of the MenuMode specialization `T`.
/// This is necessary because `MenuModeBase` needs to know `Self::transition_t`
/// during instantiation, but during that time the derived class `Self` is an
/// incomplete type.
/// This should be specialized for each specialization `T` of `MenuMode`,
/// providing a member typedef `type` equal to `T::transition_t`.
/// \ingroup OpenOblivionModes
template<class T> struct MenuModeTransition;

/// Helper type for `oo::MenuModeTransition`.
/// \ingroup OpenOblivionModes
template<class T>
using MenuModeTransition_t = typename MenuModeTransition<T>::type;

/// Base class for menu `Mode`s.
/// This class provides the common functionality for all `Mode`s which represent
/// menu states and itself models the `Mode` concept. It is intended to be
/// derived from by each specialization of `oo::MenuMode` in the manner of CRTP.
/// Then, each specialization automatically models the `Mode` concept and is
/// free to extend the functionality of the menu by providing (public)
/// implementation functions `handleEventImpl()` and `updateImpl()` with the
/// same signature as their non-impl base-class counterparts. Each derived class
/// should also implement a `getFilenameImpl()` method that returns the filepath
/// of the XML file describing the menu. The named file is opened and processed
/// automatically during the construction of the base class.
/// \ingroup OpenOblivionModes
template<class Self> class MenuModeBase {
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

  static void notifyClicked(gui::UiElement *elem) {
    if (!!(sdl::getModState() & sdl::ModifierKey::Shift)) {
      elem->notify_shiftclicked();
    } else {
      elem->notify_clicked();
    }
    std::string soundName{gui::getClicksound(elem->get_clicksound())};
    if (!soundName.empty()) {
      oo::Path soundPath{std::move(soundName)};
      Ogre::SoundManager::getSingleton().playSound(soundPath.c_str(),
                                                   oo::RESOURCE_GROUP,
                                                   "effect");
    }
  }

 protected:
  /// Find the `gui::UiElement` under the cursor and call `f` on it and its
  /// ancestors in decreasing order of generation.
  template<class F> void notifyElementAtCursor(F &&f);

  /// Get a pointer to the \ref OpenOblivionGui layer underlying the menu.
  gui::MenuContext *getMenuCtx() {
    // mMenuCtx.has_value() is guaranteed since the constructor would have
    // thrown an exception otherwise.
    return &*mMenuCtx;
  }

  /// The number of seconds elapsed since the menu was constructed.
  float getClock() const {
    return mClock;
  }

  /// Return a pointer to the element with the given id, or nullptr if no such
  /// element exists.
  /// This is only guaranteed to be `O(n)` or better.
  /// \see gui::MenuContext::getElementWithId()
  const gui::UiElement *getElementWithId(int id) const {
    return mMenuCtx->getElementWithId(id);
  }

  /// \copydoc getElementWithId()
  gui::UiElement *getElementWithId(int id) {
    return mMenuCtx->getElementWithId(id);
  }

 public:
  using transition_t = MenuModeTransition_t<Self>;

  explicit MenuModeBase(ApplicationContext &ctx);

  void enter(ApplicationContext &ctx) {
    refocus(ctx);
  }

  void refocus(ApplicationContext &) {
    sdl::setRelativeMouseMode(false);
  }

  /// Handle transfer of user input to the underlying \ref OpenOblivionGui layer
  /// and calls `Self::handleEventImpl()`.
  /// \see Mode::handleEvent()
  transition_t handleEvent(ApplicationContext &ctx, const sdl::Event &event);

  /// Update the underlying \ref OpenOblivionGui layer and call
  /// `Self::updateImpl()`.
  /// \see Mode::update()
  void update(ApplicationContext &ctx, float delta);

  /// Get the filename of the XML file that defines this menu.
  /// Calls `Self::getFilenameImpl()`.
  std::string getFilename() const {
    return self().getFilenameImpl();
  }

  void hideOverlay() {
    if (mMenuCtx) mMenuCtx->getOverlay()->hide();
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
    if (!mMenuCtx) return {false, std::nullopt};
    if (sdl::mouseButtonOf(event.button) == sdl::MouseButton::Left) {
      notifyElementAtCursor(MenuModeBase<Self>::notifyClicked);
    }
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
