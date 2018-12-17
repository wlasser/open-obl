#ifndef OPENOBLIVION_GUI_LOADING_MENU_HPP
#define OPENOBLIVION_GUI_LOADING_MENU_HPP

#include "gui/menu.hpp"
#include "gui/trait.hpp"
#include <OgreOverlay.h>
#include <OgreOverlayManager.h>
#include <string>

namespace gui {

template<>
class Menu<MenuType::LoadingMenu> : public UiElement {
 private:
  /// \name User traits
  /// These are set by the implementation through a `gui::UserTraitInterface`.
  ///@{

  /// `user0`: Position in background image slideshow.
  int mStepNumber{};
  /// `user1`: Background image to display.
  std::string mLoadImage{};
  /// `user2`: Caption to display.
  std::string mLoadText{};
  /// `user3`: Current position of the loading progress bar.
  int mCurrentProgress{};
  /// `user4`: Maximum position of the loading progress bar.
  /// That is, the value of `mCurrentProgress` when loading is complete.
  int mMaximumProgress{};
  /// `user5`: Additional text to display during debug.
  std::string mDebugText{};

  UserTraitInterface<int, std::string, std::string, int, int, std::string>
      mInterface{std::make_tuple(&mStepNumber,
                                 &mLoadImage,
                                 &mLoadText,
                                 &mCurrentProgress,
                                 &mMaximumProgress,
                                 &mDebugText)};
  ///@}

  bool mVisible{true};

  /// Parent `Ogre::Overlay` of this menu.
  Ogre::Overlay *mOverlay{};

 public:
  auto getUserOutputTraitInterface() {
    return std::make_tuple(&mStepNumber, &mLoadImage, &mLoadText,
                           &mCurrentProgress, &mMaximumProgress, &mDebugText);
  }

  BUILD_USER_TRAIT_INTERFACE(mInterface);

  void set_visible(bool visible) override {
    mVisible = visible;
    if (mOverlay) mVisible ? mOverlay->show() : mOverlay->hide();
  }

  Ogre::Overlay *getOverlay() {
    if (!mOverlay) {
      auto *overlayMgr{Ogre::OverlayManager::getSingletonPtr()};
      if (!overlayMgr) return nullptr;
      mOverlay = overlayMgr->create(this->get_name());
      // Overlays start off hidden
      if (mVisible) mOverlay->show();
    }
    return mOverlay;
  }
};

using LoadingMenu = Menu<MenuType::LoadingMenu>;

} // namespace gui

#endif // OPENOBLIVION_GUI_LOADING_MENU_HPP
