#include "gui/elements/panel_mixin.hpp"
#include "gui/screen.hpp"
#include <OgreOverlayManager.h>

gui::PanelMixin::PanelMixin(const std::string &name) {
  if (auto *overlayMgr{Ogre::OverlayManager::getSingletonPtr()}) {
    auto *overlay{overlayMgr->createOverlayElement("Panel", name)};
    mOverlay = dynamic_cast<Ogre::PanelOverlayElement *>(overlay);
    mOverlay->show();
  }
}

gui::PanelMixin::~PanelMixin() {
  if (!mOverlay) return;
  auto &overlayMgr{Ogre::OverlayManager::getSingleton()};
  overlayMgr.destroyOverlayElement(mOverlay);
}

void gui::PanelMixin::set_x(float x) {
  if (!mOverlay) return;
  const Ogre::Vector2 dims{gui::getNormalizedDimensions()};
  mOverlay->setLeft(x / dims.x);
}

void gui::PanelMixin::set_y(float y) {
  if (!mOverlay) return;
  const Ogre::Vector2 dims{gui::getNormalizedDimensions()};
  mOverlay->setTop(y / dims.y);
}

void gui::PanelMixin::set_width(float width) {
  if (!mOverlay) return;
  const Ogre::Vector2 dims{gui::getNormalizedDimensions()};
  mOverlay->setWidth(width / dims.x);
}

void gui::PanelMixin::set_height(float height) {
  if (!mOverlay) return;
  const Ogre::Vector2 dims{gui::getNormalizedDimensions()};
  mOverlay->setHeight(height / dims.y);
}

void gui::PanelMixin::set_visible(bool visible) {
  if (!mOverlay) return;
  visible ? mOverlay->show() : mOverlay->hide();
}

Ogre::OverlayElement *gui::PanelMixin::getOverlayElement() {
  return mOverlay;
}

Ogre::PanelOverlayElement *gui::PanelMixin::getPanelOverlayElement() {
  return mOverlay;
}
