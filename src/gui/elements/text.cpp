#include "gui/elements/text.hpp"
#include "gui/logging.hpp"
#include "gui/screen.hpp"
#include <OgreMaterialManager.h>
#include <OgreOverlay.h>
#include <OgreOverlayManager.h>
#include <OgrePass.h>
#include <OgreTechnique.h>
#include <OgreTextureManager.h>

// This constructor has to do some weird stuff with materials due to how
// Ogre::TextOverlayElement works.
//
// First, suppose we have an Ogre::Font declared in a resource group FONT_GROUP.
// It must be given a `source`, which is the name of a texture resource or `ttf`
// file declared in FONT_GROUP. If the source is a texture, then the font must
// also have its `type` set to `FT_IMAGE`; this is not necessary if the source
// is a `ttf` file, because the default is `FT_TRUETYPE`. For `ttf` sources,
// the font must also be given a `size` and a `resolution`.
//
// When the Ogre::Font is loaded an Ogre::Material is created whose name is the
// font name (not the `source` name) with "Fonts/" prepended. If the `type` is
// `FT_IMAGE` then the `source` texture is loaded directly and given to the
// material as its first texture unit. If the `type` is `FT_TRUETYPE` then a
// new texture whose name is the font name (not the `source` name) with
// "Texture" appended is created and given to the material as its first texture
// unit.
//
// Importantly, Ogre::Font assumes that the RTSS is being used; the material
// created does not have any shaders. Moreover, the texture is not directly
// accessible, and its name depends on the `type` of the font. We therefore need
// to either attach shaders to the font's material, or yank the texture name out
// of the font's material and use it to attach a texture unit to an existing
// material.
//
// Back to Ogre::TextOverlayElement. This stores the font's material as its own
// material, but does not load the font or set its material until `getMaterial`
// is called. Moreover, it only does so if a material has not already been set;
// since the font's material is needed to render the font, the element does not
// expect the user to go and set the material manually. We can therefore either
// load the font ourselves and do either of the above methods to avoid RTSS, or
// we can call `getMaterial` then take the texture name from the element.
gui::Text::Text(std::string name) {
  set_name(std::move(name));

  if (auto *overlayMgr{Ogre::OverlayManager::getSingletonPtr()}) {
    auto *overlay{overlayMgr->createOverlayElement("TextArea", get_name())};
    mOverlay = dynamic_cast<Ogre::TextAreaOverlayElement *>(overlay);

    auto &matMgr{Ogre::MaterialManager::getSingleton()};
    std::string matName{std::string("__GuiMaterial:") + get_name()};

    if (matMgr.resourceExists(matName, oo::SHADER_GROUP)) {
      mMatPtr = matMgr.getByName(matName, oo::SHADER_GROUP);
    } else {
      auto baseMat{matMgr.getByName("__GuiMaterial", oo::SHADER_GROUP)};
      mMatPtr = baseMat->clone(matName);
    }

    mOverlay->setFontName("fonts/kingthings_regular.fnt", oo::RESOURCE_GROUP);

    auto *fontPass{mOverlay->getMaterial()->getTechnique(0)->getPass(0)};
    auto *fontState{fontPass->getTextureUnitStates().front()};
    auto texPtr{fontState->_getTexturePtr()};

    auto *pass{mMatPtr->getTechnique(0)->getPass(0)};
    const auto &states{pass->getTextureUnitStates()};
    if (states.empty()) pass->createTextureUnitState();
    states.front()->setTexture(texPtr);

    const Ogre::ColourValue col{1.0f, 1.0f, 1.0f, 1.0f};
    mMatPtr->setDiffuse(col);
    mOverlay->setMaterialName(matName, oo::SHADER_GROUP);

    mOverlay->setCharHeight(28.0f / overlayMgr->getViewportHeight());
    mOverlay->setCaption("");
    // OverlayElement only updates the dimensions if the size of the text is
    // bigger than the current size, but the default is 1.0 by 1.0, so we need
    // to force it.
    mOverlay->setWidth(0.0f);
    mOverlay->setHeight(0.0f);
    mOverlay->show();
  }
}

gui::Text::~Text() {
  if (!mOverlay) return;
  auto &overlayMgr{Ogre::OverlayManager::getSingleton()};
  overlayMgr.destroyOverlayElement(mOverlay);
}

void gui::Text::set_alpha(float alpha) {
  if (!mOverlay) return;
  auto *pass{mMatPtr->getTechnique(0)->getPass(0)};
  auto col{pass->getDiffuse()};
  col.a = alpha / 255.0f;
  pass->setDiffuse(col);
}

void gui::Text::set_red(float red) {
  if (!mOverlay) return;
  auto *pass{mMatPtr->getTechnique(0)->getPass(0)};
  auto col{pass->getDiffuse()};
  col.r = red / 255.0f;
  pass->setDiffuse(col);
}

void gui::Text::set_green(float green) {
  if (!mOverlay) return;
  auto *pass{mMatPtr->getTechnique(0)->getPass(0)};
  auto col{pass->getDiffuse()};
  col.g = green / 255.0f;
  pass->setDiffuse(col);
}

void gui::Text::set_blue(float blue) {
  if (!mOverlay) return;
  auto *pass{mMatPtr->getTechnique(0)->getPass(0)};
  auto col{pass->getDiffuse()};
  col.b = blue / 255.0f;
  pass->setDiffuse(col);
}

void gui::Text::set_x(float x) {
  if (!mOverlay) return;
  const Ogre::Vector2 dims{gui::getNormalizedDimensions()};
  mOverlay->setLeft(x / dims.x);
}

void gui::Text::set_y(float y) {
  if (!mOverlay) return;
  const Ogre::Vector2 dims{gui::getNormalizedDimensions()};
  mOverlay->setTop(y / dims.y);
}

void gui::Text::set_depth(float depth) {
  if (!mOverlay) return;
  if (depth < 0.0f) depth = 0.0f;
  else if (depth > 65535.0f) depth = 65535.0f;
  mOverlay->_setZOrderIncrement(static_cast<unsigned short>(depth));
  mOverlay->_getOverlay()->setZOrder(mOverlay->_getOverlay()->getZOrder());
}

void gui::Text::set_visible(bool visible) {
  if (!mOverlay) return;
  visible ? mOverlay->show() : mOverlay->hide();
}

void gui::Text::set_string(std::string str) {
  if (!mOverlay) return;
  mOverlay->setCaption(str);
}

std::optional<gui::Trait<float>> gui::Text::make_width() const {
  if (!mOverlay) return Trait<float>(get_name() + ".width", 0);

  gui::TraitFun<float> fun{[overlay = mOverlay]() -> float {
    const Ogre::Vector2 dims{gui::getNormalizedDimensions()};
    return overlay->getWidth() * dims.x;
  }};

  return Trait<float>(get_name() + ".width", std::move(fun));
}

std::optional<gui::Trait<float>> gui::Text::make_height() const {
  if (!mOverlay) return Trait<float>(get_name() + ".height", 0);

  gui::TraitFun<float> fun{[overlay = mOverlay]() -> float {
    const Ogre::Vector2 dims{gui::getNormalizedDimensions()};
    return overlay->getHeight() * dims.y;
  }};

  return Trait<float>(get_name() + ".height", std::move(fun));
}

Ogre::OverlayElement *gui::Text::getOverlayElement() {
  return mOverlay;
}
