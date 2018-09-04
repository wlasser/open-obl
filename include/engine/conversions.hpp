#ifndef OPENOBLIVION_ENGINE_CONVERSIONS_HPP
#define OPENOBLIVION_ENGINE_CONVERSIONS_HPP

#include "nif/compound.hpp"
#include <OgreColourValue.h>
#include <OgreMatrix3.h>
#include <OgreVector.h>

namespace engine::conversions {

Ogre::Vector3 fromNif(const nif::compound::Vector3 &v) {
  return Ogre::Vector3{v.x, v.y, v.z};
}

Ogre::Matrix3 fromNif(const nif::compound::Matrix33 &m) {
  return Ogre::Matrix3{m.m11, m.m12, m.m13,
                       m.m21, m.m22, m.m23,
                       m.m31, m.m32, m.m33};
}

Ogre::ColourValue fromNif(const nif::compound::Color3 &c) {
  return Ogre::ColourValue(c.r, c.g, c.b);
}

Ogre::ColourValue fromNif(const nif::compound::Color4 &c) {
  return Ogre::ColourValue(c.r, c.g, c.b, c.a);
}

Ogre::Vector3 fromBSCoordinates(const Ogre::Vector3 &v) {
  return {v.x, v.z, -v.y};
}

Ogre::Matrix3 fromBSCoordinates(const Ogre::Matrix3 &m) {
  const Ogre::Matrix3 C = {1, 0, 0,
                           0, 0, 1,
                           0, -1, 0};
  return C * m * C.transpose();
}

} // namespace engine::conversions

#endif //OPENOBLIVION_ENGINE_CONVERSIONS_HPP
