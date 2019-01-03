#include "gui/elements/rect.hpp"
#include "gui/screen.hpp"
#include <OgreOverlayManager.h>

gui::Rect::Rect(std::string name) {
  set_name(std::move(name));

  if (auto *overlayMgr{Ogre::OverlayManager::getSingletonPtr()}) {
    mOverlay = dynamic_cast<Ogre::PanelOverlayElement *>(
        overlayMgr->createOverlayElement("Panel", get_name()));
    mOverlay->show();
  }
}

gui::Rect::~Rect() {
  if (mOverlay) {
    auto &overlayMgr{Ogre::OverlayManager::getSingleton()};
    overlayMgr.destroyOverlayElement(mOverlay);
  }
}

void gui::Rect::set_x(float x) {
  if (!mOverlay) return;
  const Ogre::Vector2 dims{gui::getNormalizedDimensions()};
  mOverlay->setLeft(x / dims.x);
}

void gui::Rect::set_y(float y) {
  if (!mOverlay) return;
  const Ogre::Vector2 dims{gui::getNormalizedDimensions()};
  mOverlay->setTop(y / dims.y);
}

void gui::Rect::set_width(float width) {
  if (!mOverlay) return;
  const Ogre::Vector2 dims{gui::getNormalizedDimensions()};
  mOverlay->setWidth(width / dims.x);
}

void gui::Rect::set_height(float height) {
  if (!mOverlay) return;
  const Ogre::Vector2 dims{gui::getNormalizedDimensions()};
  mOverlay->setHeight(height / dims.y);
}

void gui::Rect::set_visible(bool visible) {
  if (!mOverlay) return;
  if (visible) mOverlay->show();
  else mOverlay->hide();
}

void gui::Rect::set_target(bool isTarget) {
  mIsTarget = isTarget;
}

void gui::Rect::set_id(float id) {
  mId = static_cast<int>(id);
}

std::optional<gui::Trait<float>> gui::Rect::make_clicked() const {
  // TODO: Write make_clicked traitfun
  gui::TraitFun<float> fun{[]() -> float { return 0.0f; }};
  return gui::Trait<float>(get_name() + ".clicked", std::move(fun));
}

std::optional<gui::Trait<float>> gui::Rect::make_shiftclicked() const {
  // TODO: Write make_shiftclicked traitfun
  gui::TraitFun<float> fun{[]() -> float { return 0.0f; }};
  return gui::Trait<float>(get_name() + ".shiftclicked", std::move(fun));
}

std::optional<gui::Trait<float>> gui::Rect::make_mouseover() const {
  // TODO: Write make_mouseover traitfun
  gui::TraitFun<float> fun{[]() -> float { return 0.0f; }};
  return gui::Trait<float>(get_name() + ".mouseover", std::move(fun));
}
