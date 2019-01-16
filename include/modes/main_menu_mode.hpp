#ifndef OPENOBLIVION_MAIN_MENU_MODE_HPP
#define OPENOBLIVION_MAIN_MENU_MODE_HPP

#include "modes/menu_mode.hpp"
#include "modes/menu_mode_base.hpp"
#include <gsl/gsl>
#include <OgreCamera.h>
#include <OgreSceneManager.h>

namespace oo {

/// \ingroup OpenOblivionModes
template<> struct MenuModeTransition<MainMenuMode> {
  using type = ModeTransition<MainMenuMode, LoadMenuMode, LoadingMenuMode,
                              GameMode>;
};

/// Specialization of `oo::MenuMode` for the Main Menu.
/// \ingroup OpenOblivionModes
template<> class MenuMode<gui::MenuType::MainMenu>
    : public MenuModeBase<MainMenuMode> {
 private:
  /// This mode gets its own scene manager because it is opened before any game
  /// has been loaded.
  Ogre::SceneManager *mScnMgr{};

  /// Type of the `Ogre::SceneManager` to use for this mode.
  constexpr static const char *SCN_MGR_TYPE{"DefaultSceneManager"};

  /// Name of the `Ogre::SceneManager` to use for this mode.
  constexpr static const char *SCN_MGR_NAME{"__MainMenuSceneManager"};

  Ogre::Camera *mCamera{};

  /// Name of the `Ogre::Camera` to use for this mode.
  constexpr static const char *CAMERA_NAME{"__MainMenuCamera"};

  /// `<id> 2 </id>`
  const gui::UiElement *btnContinue{};

  /// `<id> 3 </id>`
  const gui::UiElement *btnNew{};

  /// `<id> 4 </id>`
  const gui::UiElement *btnLoad{};

  /// `<id> 5 </id>`
  const gui::UiElement *btnOptions{};

  /// `<id> 6 </id>`
  const gui::UiElement *btnCredits{};

  /// `<id> 7 </id>`
  const gui::UiElement *btnExit{};

 public:
  explicit MenuMode<gui::MenuType::MainMenu>(ApplicationContext &ctx)
      : MenuModeBase<MainMenuMode>(ctx),
        mScnMgr{ctx.getRoot().createSceneManager(SCN_MGR_TYPE, SCN_MGR_NAME)},
        mCamera{mScnMgr->createCamera(CAMERA_NAME)},
        btnContinue{getMenuCtx()->getElementWithId(2)},
        btnNew{getMenuCtx()->getElementWithId(3)},
        btnLoad{getMenuCtx()->getElementWithId(4)},
        btnOptions{getMenuCtx()->getElementWithId(5)},
        btnCredits{getMenuCtx()->getElementWithId(6)},
        btnExit{getMenuCtx()->getElementWithId(7)} {
    mScnMgr->addRenderQueueListener(ctx.getImGuiManager());
    mScnMgr->addRenderQueueListener(ctx.getOverlaySystem());
    ctx.setCamera(gsl::make_not_null(mCamera));
  }

  ~MenuMode<gui::MenuType::MainMenu>() {
    auto *root{Ogre::Root::getSingletonPtr()};
    if (root && mScnMgr) root->destroySceneManager(mScnMgr);
  }

  MenuMode(const MenuMode &) = delete;
  MenuMode &operator=(const MenuMode &) = delete;

  MenuMode(MenuMode &&other) noexcept
      : MenuModeBase<MainMenuMode>(std::move(other)),
        mScnMgr(other.mScnMgr),
        mCamera(other.mCamera),
        btnContinue(other.btnContinue),
        btnNew(other.btnNew),
        btnLoad(other.btnLoad),
        btnOptions(other.btnOptions),
        btnCredits(other.btnCredits),
        btnExit(other.btnExit) {
    other.mScnMgr = nullptr;
    other.mCamera = nullptr;
  }

  MenuMode &operator=(MenuMode &&other) noexcept {
    if (this != &other) {
      auto tmp{std::move(other)};
      std::swap(*this, tmp);
    }
    return *this;
  }

  std::string getFilenameImpl() const {
    return "menus/options/main_menu.xml";
  }

  MainMenuMode::transition_t
  handleEventImpl(ApplicationContext &ctx, const sdl::Event &event);

  void updateImpl(ApplicationContext &ctx, float delta);
};

} // namespace oo

#endif // OPENOBLIVION_MAIN_MENU_MODE_HPP
