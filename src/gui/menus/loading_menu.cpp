#include "gui/logging.hpp"
#include "gui/menus/loading_menu.hpp"
#include "gui/screen.hpp"
#include <OgreOverlay.h>
#include <OgreOverlayManager.h>
#include <OgrePanelOverlayElement.h>

gui::Menu<gui::MenuType::LoadingMenu>::~Menu<gui::MenuType::LoadingMenu>() {
  if (auto *overlayMgr{Ogre::OverlayManager::getSingletonPtr()}) {
    if (mOverlayContainer) {
      overlayMgr->destroyOverlayElement(mOverlayContainer);
    }
    if (mOverlay) {
      overlayMgr->destroy(mOverlay);
    }
  }
};

void gui::LoadingMenu::set_visible(bool visible) {
  mVisible = visible;
  if (mOverlay) mVisible ? mOverlay->show() : mOverlay->hide();
}

Ogre::Overlay *gui::LoadingMenu::getOverlay() {
  if (!mOverlay) {
    auto *overlayMgr{Ogre::OverlayManager::getSingletonPtr()};
    if (!overlayMgr) return nullptr;
    mOverlay = overlayMgr->create(this->get_name());
    // Overlays start off hidden
    if (mVisible) mOverlay->show();
  }
  return mOverlay;
}

Ogre::OverlayElement *gui::LoadingMenu::getOverlayElement() {
  if (!mOverlayContainer) {
    auto *overlay{getOverlay()};
    if (!overlay) return nullptr;

    auto *overlayMgr{Ogre::OverlayManager::getSingletonPtr()};
    if (!overlayMgr) return nullptr;

    mOverlayContainer = dynamic_cast<Ogre::PanelOverlayElement *>(
        overlayMgr->createOverlayElement("Panel", this->get_name()));
    overlay->add2D(mOverlayContainer);

    mOverlayContainer->setMetricsMode(Ogre::GuiMetricsMode::GMM_PIXELS);
    Ogre::Vector2 dims{gui::getNormalizedDimensions()};
    mOverlayContainer->setDimensions(dims.x, dims.y);
    gui::guiLogger()->info("Created overlay container ({} x {})",
                           dims.x, dims.y);
    mOverlayContainer->setPosition(0.0f, 0.0f);
    mOverlayContainer->show();
  }
  return mOverlayContainer;
}