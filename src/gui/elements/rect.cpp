#include "gui/elements/rect.hpp"
#include "gui/screen.hpp"
#include <OgreOverlayManager.h>

gui::Rect::Rect(std::string name) {
  set_name(std::move(name));

  if (auto *overlayMgr{Ogre::OverlayManager::getSingletonPtr()}) {
    mOverlay = dynamic_cast<Ogre::PanelOverlayElement *>(
        overlayMgr->createOverlayElement("Panel", get_name()));
  }
}

gui::Rect::~Rect() {
  if (mOverlay) {
    auto &overlayMgr{Ogre::OverlayManager::getSingleton()};
    overlayMgr.destroyOverlayElement(mOverlay);
  }
}

void gui::Rect::set_x(int x) {
  if (!mOverlay) return;
  const Ogre::Vector2 dims{gui::getNormalizedDimensions()};
  mOverlay->setLeft(static_cast<Ogre::Real>(x) / dims.x);
}

void gui::Rect::set_y(int y) {
  if (!mOverlay) return;
  const Ogre::Vector2 dims{gui::getNormalizedDimensions()};
  mOverlay->setTop(static_cast<Ogre::Real>(y) / dims.y);
}

void gui::Rect::set_width(int width) {
  if (!mOverlay) return;
  const Ogre::Vector2 dims{gui::getNormalizedDimensions()};
  mOverlay->setWidth(static_cast<Ogre::Real>(width) / dims.x);
}

void gui::Rect::set_height(int height) {
  if (!mOverlay) return;
  const Ogre::Vector2 dims{gui::getNormalizedDimensions()};
  mOverlay->setHeight(static_cast<Ogre::Real>(height) / dims.y);
}

void gui::Rect::set_visible(bool visible) {
  if (!mOverlay) return;
  if (visible) mOverlay->show();
  else mOverlay->hide();
}