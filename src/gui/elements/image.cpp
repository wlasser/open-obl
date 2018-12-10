#include "fs/path.hpp"
#include "gui/elements/image.hpp"
#include "gui/screen.hpp"
#include "settings.hpp"
#include <OgreMaterial.h>
#include <OgreOverlayManager.h>
#include <OgrePass.h>
#include <OgreTechnique.h>
#include <string>

gui::Image::Image(std::string name) {
  set_name(std::move(name));

  if (auto *overlayMgr{Ogre::OverlayManager::getSingletonPtr()}) {
    mOverlay = dynamic_cast<Ogre::PanelOverlayElement *>(
        overlayMgr->createOverlayElementFromFactory("Panel", get_name()));
    mOverlay->setMaterialName("__GuiMaterial", oo::SHADER_GROUP);
  }
}

void gui::Image::set_x(int x) {
  if (!mOverlay) return;
  const Ogre::Vector2 dims{gui::getNormalizedDimensions()};
  mOverlay->setLeft(static_cast<Ogre::Real>(x) / dims.x);
}

void gui::Image::set_y(int y) {
  if (!mOverlay) return;
  const Ogre::Vector2 dims{gui::getNormalizedDimensions()};
  mOverlay->setTop(static_cast<Ogre::Real>(y) / dims.y);
}

void gui::Image::set_width(int width) {
  if (!mOverlay) return;
  const Ogre::Vector2 dims{gui::getNormalizedDimensions()};
  mOverlay->setWidth(static_cast<Ogre::Real>(width) / dims.x);
}

void gui::Image::set_height(int height) {
  if (!mOverlay) return;
  const Ogre::Vector2 dims{gui::getNormalizedDimensions()};
  mOverlay->setHeight(static_cast<Ogre::Real>(height) / dims.y);
}

void gui::Image::set_filename(std::string filename) {
  auto *pass{mOverlay->getMaterial()->getTechnique(0)->getPass(0)};
  const auto &states{pass->getTextureUnitStates()};
  if (states.empty()) {
    pass->createTextureUnitState(oo::Path{std::move(filename)}.c_str());
  } else {
    auto *state{pass->getTextureUnitStates()[0]};
    state->setTextureName(oo::Path{std::move(filename)}.c_str());
  }
}

void gui::Image::set_zoom(int zoom) {
  // TODO: Unimplemented
}