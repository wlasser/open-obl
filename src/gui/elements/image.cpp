#include "fs/path.hpp"
#include "gui/elements/image.hpp"
#include "gui/screen.hpp"
#include "settings.hpp"
#include <OgreMaterial.h>
#include <OgreMaterialManager.h>
#include <OgreOverlayManager.h>
#include <OgrePass.h>
#include <OgreTechnique.h>
#include <OgreTextureManager.h>
#include <string>

gui::Image::Image(std::string name) {
  set_name(std::move(name));

  if (auto *overlayMgr{Ogre::OverlayManager::getSingletonPtr()}) {
    mOverlay = dynamic_cast<Ogre::PanelOverlayElement *>(
        overlayMgr->createOverlayElement("Panel", get_name()));

    auto *matMgr{Ogre::MaterialManager::getSingletonPtr()};
    std::string matName{std::string("__GuiMaterial:") + get_name()};

    if (matMgr->resourceExists(matName, oo::SHADER_GROUP)) {
      mMatPtr = matMgr->getByName(matName, oo::SHADER_GROUP);
    } else {
      auto baseMat{matMgr->getByName("__GuiMaterial", oo::SHADER_GROUP)};
      mMatPtr = baseMat->clone("__GuiMaterial:" + get_name());
    }

    mOverlay->setMaterialName(mMatPtr->getName(), oo::SHADER_GROUP);
  }
}

gui::Image::~Image() {
  if (mOverlay) {
    auto &overlayMgr{Ogre::OverlayManager::getSingleton()};
    overlayMgr.destroyOverlayElement(mOverlay);
  }
}

void gui::Image::set_x(float x) {
  if (!mOverlay) return;
  const Ogre::Vector2 dims{gui::getNormalizedDimensions()};
  mOverlay->setLeft(x / dims.x);
}

void gui::Image::set_y(float y) {
  if (!mOverlay) return;
  const Ogre::Vector2 dims{gui::getNormalizedDimensions()};
  mOverlay->setTop(y / dims.y);
}

void gui::Image::set_width(float width) {
  if (!mOverlay) return;
  const Ogre::Vector2 dims{gui::getNormalizedDimensions()};
  mOverlay->setWidth(width / dims.x);
}

void gui::Image::set_height(float height) {
  if (!mOverlay) return;
  const Ogre::Vector2 dims{gui::getNormalizedDimensions()};
  mOverlay->setHeight(height / dims.y);
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

void gui::Image::set_zoom(float zoom) {
  // TODO: Unimplemented
}

void gui::Image::set_visible(bool visible) {
  if (!mOverlay) return;
  if (visible) mOverlay->show();
  else mOverlay->hide();
}