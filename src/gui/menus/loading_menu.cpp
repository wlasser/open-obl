#include "gui/logging.hpp"
#include "gui/menus/loading_menu.hpp"
#include "gui/screen.hpp"
#include <OgreOverlay.h>
#include <OgreOverlayManager.h>
#include <OgrePanelOverlayElement.h>

namespace gui {

Menu<MenuType::LoadingMenu>::~Menu() {
  if (auto *overlayMgr{Ogre::OverlayManager::getSingletonPtr()}) {
    if (mOverlayContainer) {
      overlayMgr->destroyOverlayElement(mOverlayContainer);
    }
    if (mOverlay) {
      overlayMgr->destroy(mOverlay);
    }
  }
}

Menu<MenuType::LoadingMenu>::Menu() {
  if (auto *overlayMgr{Ogre::OverlayManager::getSingletonPtr()}) {
    mOverlay = overlayMgr->create("__LoadingMenuOverlay");
    mOverlay->show();

    mOverlayContainer = dynamic_cast<Ogre::PanelOverlayElement *>(
        overlayMgr->createOverlayElement("Panel", "__LoadingMenuContainer"));
    mOverlayContainer->getUserObjectBindings().setUserAny<UiElement *>(this);
    mOverlay->add2D(mOverlayContainer);

    mOverlayContainer->setMetricsMode(Ogre::GuiMetricsMode::GMM_PIXELS);
    Ogre::Vector2 dims{gui::getNormalizedDimensions()};
    mOverlayContainer->setDimensions(dims.x, dims.y);
    gui::guiLogger()->info("Created overlay container ({} x {})",
                           dims.x, dims.y);
    mOverlayContainer->setPosition(0.0f, 0.0f);
    mOverlayContainer->show();
  }
}

void LoadingMenu::set_visible(bool visible) {
  mVisible = visible;
  if (mOverlay) mVisible ? mOverlay->show() : mOverlay->hide();
}

Ogre::Overlay *LoadingMenu::getOverlay() const {
  return mOverlay;
}

Ogre::OverlayElement *LoadingMenu::getOverlayElement() const {
  return mOverlayContainer;
}

} // namespace gui