#include "gui/logging.hpp"
#include "gui/menus/load_menu.hpp"
#include "gui/screen.hpp"
#include <OgreOverlay.h>
#include <OgreOverlayManager.h>
#include <OgrePanelOverlayElement.h>

gui::Menu<gui::MenuType::LoadMenu>::~Menu<gui::MenuType::LoadMenu>() {
  if (auto *overlayMgr{Ogre::OverlayManager::getSingletonPtr()}) {
    if (mOverlayContainer) {
      overlayMgr->destroyOverlayElement(mOverlayContainer);
    }
    if (mOverlay) {
      overlayMgr->destroy(mOverlay);
    }
  }
};

gui::Menu<gui::MenuType::LoadMenu>::Menu() {
  if (auto *overlayMgr{Ogre::OverlayManager::getSingletonPtr()}) {
    mOverlay = overlayMgr->create("__LoadMenuOverlay");
    mOverlay->show();

    mOverlayContainer = dynamic_cast<Ogre::PanelOverlayElement *>(
        overlayMgr->createOverlayElement("Panel", "__LoadMenuContainer"));
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

void gui::LoadMenu::set_visible(bool visible) {
  mVisible = visible;
  if (mOverlay) mVisible ? mOverlay->show() : mOverlay->hide();
}

std::optional<gui::Trait<float>> gui::LoadMenu::make_x() const {
  return gui::Trait<float>(get_name() + ".x", 0);
}

std::optional<gui::Trait<float>> gui::LoadMenu::make_y() const {
  return gui::Trait<float>(get_name() + ".y", 0);
}

Ogre::Overlay *gui::LoadMenu::getOverlay() const {
  return mOverlay;
}

Ogre::OverlayElement *gui::LoadMenu::getOverlayElement() const {
  return mOverlayContainer;
}
