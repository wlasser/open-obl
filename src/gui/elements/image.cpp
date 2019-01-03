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
  if (mZoom > 0.0f) {
    const float zoom{mZoom / 100.0f};
    mOverlay->setUV(0.0f, 0.0f,
                    zoom * mOverlay->getWidth() * dims.x / mTexWidth,
                    zoom * mOverlay->getHeight() * dims.y / mTexHeight);
  } else {
    mOverlay->setUV(0.0f, 0.0f, 1.0f, 1.0f);
  }
}

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
  updateUVs(dims);
}

void gui::Image::set_height(float height) {
  if (!mOverlay) return;
  const Ogre::Vector2 dims{gui::getNormalizedDimensions()};
  mOverlay->setHeight(height / dims.y);
  updateUVs(dims);
}

void gui::Image::set_alpha(float alpha) {
  if (!mOverlay) return;
  const Ogre::ColourValue col{1.0f, 1.0f, 1.0f, alpha / 255.0f};
  mMatPtr->setDiffuse(col);
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

  mTexWidth = static_cast<float>(texPtr->getWidth());
  mTexHeight = static_cast<float>(texPtr->getHeight());
  updateUVs(gui::getNormalizedDimensions());
}

void gui::Image::set_zoom(float zoom) {
  mZoom = zoom;
  updateUVs(gui::getNormalizedDimensions());
}

void gui::Image::set_visible(bool visible) {
  if (!mOverlay) return;
  if (visible) mOverlay->show();
  else mOverlay->hide();
}

void gui::Image::set_target(bool isTarget) {
  mIsTarget = isTarget;
}

void gui::Image::set_id(float id) {
  mId = static_cast<int>(id);
}

// Note: Cannot check mIsTarget && mId >= -1 on construction because these
// values are not set until the first update(), which must occur after all the
// traits have been added. Thus the traits must always be added, and their
// behaviour must depend on the condition.
std::optional<gui::Trait<float>> gui::Image::make_clicked() const {
  // TODO: Write make_clicked traitfun
  gui::TraitFun<float> fun{[]() -> float { return 0.0f; }};
  return gui::Trait<float>(get_name() + ".clicked", std::move(fun));
}

std::optional<gui::Trait<float>> gui::Image::make_shiftclicked() const {
  // TODO: Write make_shiftclicked traitfun
  gui::TraitFun<float> fun{[]() -> float { return 0.0f; }};
  return gui::Trait<float>(get_name() + ".shiftclicked", std::move(fun));
}

std::optional<gui::Trait<float>> gui::Image::make_mouseover() const {
  // TODO: Write make_mouseover traitfun
  gui::TraitFun<float> fun{[]() -> float { return 0.0f; }};
  return gui::Trait<float>(get_name() + ".mouseover", std::move(fun));
}
