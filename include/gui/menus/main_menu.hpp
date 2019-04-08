#ifndef OPENOBLIVION_GUI_MAIN_MENU_HPP
#define OPENOBLIVION_GUI_MAIN_MENU_HPP

#include "gui/menu.hpp"

namespace gui {

template<>
class Menu<MenuType::MainMenu> : public UiElement {
 private:
  /// \name User traits
  /// These are set by the implementation through a `gui::UserTraitInterface`.
  ///@{

  /// `user0`: Whether the logo is visible.
  bool mIsMenuVisible{};
  /// `user1`: Whether the button are visible.
  bool mAreButtonsVisible{};
  /// `user2`: Whether 'Press Start' is visible.
  bool mIsPressStartVisible{};
  /// `user3`: Alpha value of logo.
  float mLogoAlpha{};
  /// `user4`: Length of logo transition in seconds.
  float mLogoTransitionLength{};

  UserTraitInterface<bool, bool, bool, float, float>
      mInterface{std::make_tuple(&mIsMenuVisible,
                                 &mAreButtonsVisible,
                                 &mIsPressStartVisible,
                                 &mLogoAlpha,
                                 &mLogoTransitionLength)};
  ///@}

  bool mVisible{false};

  /// Parent `Ogre::Overlay` of this menu
  Ogre::Overlay *mOverlay{};

  /// Toplevel containr for `Ogre::OverlayElement`s.
  Ogre::OverlayContainer *mOverlayContainer{};

 public:
  Menu<MenuType::MainMenu>();
  ~Menu<MenuType::MainMenu>() override;

  auto getUserOutputTraitInterface() const {
    return std::make_tuple(&mIsMenuVisible,
                           &mAreButtonsVisible,
                           &mIsPressStartVisible,
                           &mLogoAlpha,
                           static_cast<float *>(nullptr));
  }

  BUILD_USER_TRAIT_INTERFACE(mInterface);

  void set_visible(bool visible) override;

  Ogre::Overlay *getOverlay() const;
  Ogre::OverlayElement *getOverlayElement() const override;
};

using MainMenu = Menu<MenuType::MainMenu>;

} // namespace gui

#endif //OPENOBLIVION_GUI_MAIN_MENU_HPP
