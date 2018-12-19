#ifndef OPENOBLIVION_GUI_LOADING_MENU_HPP
#define OPENOBLIVION_GUI_LOADING_MENU_HPP

#include "gui/menu.hpp"
#include "gui/trait.hpp"
#include <OgreOverlay.h>
#include <OgrePanelOverlayElement.h>
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
  float mStepNumber{};
  /// `user1`: Background image to display.
  std::string mLoadImage{};
  /// `user2`: Caption to display.
  std::string mLoadText{};
  /// `user3`: Current position of the loading progress bar.
  float mCurrentProgress{};
  /// `user4`: Maximum position of the loading progress bar.
  /// That is, the value of `mCurrentProgress` when loading is complete.
  float mMaximumProgress{};
  /// `user5`: Additional text to display during debug.
  std::string mDebugText{};

  UserTraitInterface<float, std::string, std::string, float, float, std::string>
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

  /// Toplevel container for `Ogre::OverlayElement`s.
  Ogre::OverlayContainer *mOverlayContainer{};

 public:
  ~Menu<MenuType::LoadingMenu>() override {
    if (auto *overlayMgr{Ogre::OverlayManager::getSingletonPtr()}) {
      if (mOverlayContainer) {
        overlayMgr->destroyOverlayElement(mOverlayContainer);
      }
      if (mOverlay) {
        overlayMgr->destroy(mOverlay);
      }
    }
  }

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

  Ogre::OverlayElement *getOverlayElement() override {
    if (!mOverlayContainer) {
      auto *overlay{getOverlay()};
      if (!overlay) return nullptr;

      auto *overlayMgr{Ogre::OverlayManager::getSingletonPtr()};
      if (!overlayMgr) return nullptr;

      mOverlayContainer = dynamic_cast<Ogre::PanelOverlayElement *>(
          overlayMgr->createOverlayElement("Panel", this->get_name()));
      overlay->add2D(mOverlayContainer);

      mOverlayContainer->setDimensions(1.0f, 1.0f);
      mOverlayContainer->setPosition(0.0f, 0.0f);
      mOverlayContainer->show();
    }
    return mOverlayContainer;
  }
};

using LoadingMenu = Menu<MenuType::LoadingMenu>;

} // namespace gui

#endif // OPENOBLIVION_GUI_LOADING_MENU_HPP
