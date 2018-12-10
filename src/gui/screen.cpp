#include "settings.hpp"
#include <OgreRenderTarget.h>
#include <OgreRoot.h>
#include <OgreVector.h>
#include "gui/screen.hpp"

Ogre::Vector2 gui::getNormalizedDimensions() {
  auto *target{Ogre::Root::getSingleton().getRenderTarget(oo::RENDER_TARGET)};
  if (!target) return Ogre::Vector2::ZERO;
  const int rawW{static_cast<int>(target->getWidth())};
  const int rawH{static_cast<int>(target->getHeight())};
  const int w{rawW >= rawH ? ((960 * rawW) / rawH) : 1280};
  const int h{rawW < rawH ? ((1280 * rawH) / rawW) : 960};
  return {static_cast<Ogre::Real>(w), static_cast<Ogre::Real>(h)};
}

gui::ScreenElement::Dimensions
gui::ScreenElement::getNormalizedDimensions() const {
  const int rawW{mRawWidth};
  const int rawH{mRawHeight};
  const int width{rawW >= rawH ? ((960 * rawW) / rawH) : 1280};
  const int height{rawW < rawH ? ((1280 * rawH) / rawW) : 960};
  return {width, height};
}

gui::ScreenElement::ScreenElement() {
  auto *root{Ogre::Root::getSingletonPtr()};
  if (!root) return;
  auto *target{root->getRenderTarget(oo::RENDER_TARGET)};
  if (!target) return;
  mRawWidth = static_cast<int>(target->getWidth());
  mRawHeight = static_cast<int>(target->getHeight());
}