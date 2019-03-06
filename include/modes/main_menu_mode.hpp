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
  using type = ModeTransition<MainMenuMode, LoadMenuMode, LoadingMenuMode>;
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

  std::optional<Ogre::SoundHandle> mBackgroundMusic{};

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
  explicit MenuMode<gui::MenuType::MainMenu>(ApplicationContext &ctx);
  ~MenuMode<gui::MenuType::MainMenu>();

  MenuMode<gui::MenuType::MainMenu>(const MenuMode &) = delete;
  MenuMode<gui::MenuType::MainMenu> &operator=(const MenuMode &) = delete;

  MenuMode<gui::MenuType::MainMenu>(MenuMode &&other) noexcept;
  MenuMode<gui::MenuType::MainMenu> &operator=(MenuMode &&other) noexcept;

  std::string getFilenameImpl() const {
    return "menus/options/main_menu.xml";
  }

  MainMenuMode::transition_t
  handleEventImpl(ApplicationContext &ctx, const sdl::Event &event);

  void updateImpl(ApplicationContext &ctx, float delta);
};

} // namespace oo

#endif // OPENOBLIVION_MAIN_MENU_MODE_HPP
