#include "gui/logging.hpp"
#include "gui/menus/main_menu.hpp"
#include "gui/screen.hpp"
#include <OgreOverlay.h>
#include <OgreOverlayManager.h>
#include <OgrePanelOverlayElement.h>

namespace gui {

Menu<MenuType::MainMenu>::~Menu() {
  if (auto *overlayMgr{Ogre::OverlayManager::getSingletonPtr()}) {
    if (mOverlayContainer) {
      overlayMgr->destroyOverlayElement(mOverlayContainer);
    }
    if (mOverlay) {
      overlayMgr->destroy(mOverlay);
    }
  }
}

Menu<MenuType::MainMenu>::Menu() {
  if (auto *overlayMgr{Ogre::OverlayManager::getSingletonPtr()}) {
    mOverlay = overlayMgr->create("__MainMenuOverlay");
    mOverlay->show();

    mOverlayContainer = dynamic_cast<Ogre::PanelOverlayElement *>(
        overlayMgr->createOverlayElement("Panel", "__MainMenuContainer"));
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

void MainMenu::set_visible(bool visible) {
  mVisible = visible;
  if (mOverlay) mVisible ? mOverlay->show() : mOverlay->hide();
}

Ogre::Overlay *MainMenu::getOverlay() const {
  return mOverlay;
}

Ogre::OverlayElement *MainMenu::getOverlayElement() const {
  return mOverlayContainer;
}

} // namespace gui
