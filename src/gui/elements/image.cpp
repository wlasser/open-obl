#include "fs/path.hpp"
#include "gui/elements/image.hpp"
#include "gui/logging.hpp"
#include "gui/screen.hpp"
#include "settings.hpp"
#include <OgreMaterial.h>
#include <OgreMaterialManager.h>
#include <OgreOverlayManager.h>
#include <OgrePass.h>
#include <OgreTechnique.h>
#include <OgreTextureManager.h>
#include <string>

void gui::Image::updateUVs(const Ogre::Vector2 &dims) {
  auto *overlay{getPanelOverlayElement()};
  if (mZoom > 0.0f) {
    const float zoom{mZoom / 100.0f};
    overlay->setUV(0.0f, 0.0f,
                   zoom * overlay->getWidth() * dims.x / mTexWidth,
                   zoom * overlay->getHeight() * dims.y / mTexHeight);
  } else {
    overlay->setUV(0.0f, 0.0f, 1.0f, 1.0f);
  }
}

gui::Image::Image(std::string name) : PanelMixin(name) {
  set_name(std::move(name));

  if (auto *overlayMgr{Ogre::OverlayManager::getSingletonPtr()}) {
    auto *matMgr{Ogre::MaterialManager::getSingletonPtr()};
    std::string matName{std::string("__GuiMaterial:") + get_name()};

    if (matMgr->resourceExists(matName, oo::SHADER_GROUP)) {
      mMatPtr = matMgr->getByName(matName, oo::SHADER_GROUP);
    } else {
      auto baseMat{matMgr->getByName("__GuiMaterial", oo::SHADER_GROUP)};
      mMatPtr = baseMat->clone("__GuiMaterial:" + get_name());
    }

    getOverlayElement()->setMaterialName(mMatPtr->getName(), oo::SHADER_GROUP);
  }
}

void gui::Image::set_width(float width) {
  PanelMixin::set_width(width);
  if (getOverlayElement()) updateUVs(gui::getNormalizedDimensions());
}

void gui::Image::set_height(float height) {
  PanelMixin::set_height(height);
  if (getOverlayElement()) updateUVs(gui::getNormalizedDimensions());
}

void gui::Image::set_alpha(float alpha) {
  if (!getOverlayElement()) return;
  const Ogre::ColourValue col{1.0f, 1.0f, 1.0f, alpha / 255.0f};
  mMatPtr->setDiffuse(col);
}

void gui::Image::set_filename(std::string filename) {
  auto *overlay{getOverlayElement()};
  if (!overlay) return;
  auto *pass{overlay->getMaterial()->getTechnique(0)->getPass(0)};
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
  texPtr->load();

  mTexWidth = static_cast<float>(texPtr->getWidth());
  mTexHeight = static_cast<float>(texPtr->getHeight());
  updateUVs(gui::getNormalizedDimensions());
}

void gui::Image::set_zoom(float zoom) {
  mZoom = zoom;
  if (getOverlayElement()) updateUVs(gui::getNormalizedDimensions());
}

std::optional<gui::Trait<float>> gui::Image::make_filewidth() const {
  if (!getOverlayElement()) {
    return gui::Trait<float>(get_name() + ".filewidth", 0.0f);
  }

  // The texture size might change if the filename does, but the material is
  // consistent so grab the size through that.
  auto *pass{mMatPtr->getTechnique(0)->getPass(0)};
  gui::TraitFun<float> fun{[pass]() -> float {
    const auto &states{pass->getTextureUnitStates()};
    if (states.empty()) return 0.0f;
    return states[0]->getTextureDimensions().first;
  }};

  return gui::Trait<float>(get_name() + ".filewidth", std::move(fun));
}

std::optional<gui::Trait<float>> gui::Image::make_fileheight() const {
  if (!getOverlayElement()) {
    return gui::Trait<float>(get_name() + ".fileheight", 0.0f);
  }

  auto *pass{mMatPtr->getTechnique(0)->getPass(0)};
  gui::TraitFun<float> fun{[pass]() -> float {
    const auto &states{pass->getTextureUnitStates()};
    if (states.empty()) return 0.0f;
    return states[0]->getTextureDimensions().second;
  }};

  return gui::Trait<float>(get_name() + ".fileheight", std::move(fun));
}
