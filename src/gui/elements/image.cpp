#include "fs/path.hpp"
#include "gui/elements/image.hpp"
#include "gui/screen.hpp"
#include "settings.hpp"
#include <OgreMaterial.h>
#include <OgreOverlayManager.h>
#include <OgrePass.h>
#include <OgreTechnique.h>
#include <OgreTextureManager.h>
#include <spdlog/spdlog.h>
#include <string>

gui::Image::Image(std::string name) {
  set_name(std::move(name));

  if (auto *overlayMgr{Ogre::OverlayManager::getSingletonPtr()}) {
    mOverlay = dynamic_cast<Ogre::PanelOverlayElement *>(
        overlayMgr->createOverlayElement("Panel", get_name()));
    mOverlay->setMaterialName("__GuiMaterial", oo::SHADER_GROUP);
  }
}

gui::Image::~Image() {
  if (mOverlay) {
    auto &overlayMgr{Ogre::OverlayManager::getSingleton()};
    overlayMgr.destroyOverlayElement(mOverlay);
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
  if (!mOverlay) return;
  auto *pass{mOverlay->getMaterial()->getTechnique(0)->getPass(0)};
  const auto &states{pass->getTextureUnitStates()};
  const oo::Path base{"textures/"};
  const oo::Path path{base / oo::Path{std::move(filename)}};

  if (states.empty()) pass->createTextureUnitState();

  // Can't use setTextureName because it assumes the resource group of the
  // texture is the same as the resource group of the material.
  auto *state{pass->getTextureUnitStates()[0]};
  auto &texMgr{Ogre::TextureManager::getSingleton()};
  auto texPtr{texMgr.getByName(path.c_str(), oo::RESOURCE_GROUP)};
  state->setTexture(texPtr);
}

void gui::Image::set_zoom(int zoom) {
  // TODO: Unimplemented
}