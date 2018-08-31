#ifndef OPENOBLIVION_ENGINE_CONVERSIONS_HPP
#define OPENOBLIVION_ENGINE_CONVERSIONS_HPP

#include "nif/compound.hpp"
#include <OgreVector.h>
#include <OgreMatrix3.h>

namespace engine::conversions {

Ogre::Vector3 fromNif(const nif::compound::Vector3 &v) {
  return Ogre::Vector3{v.x, v.y, v.z};
}

Ogre::Matrix3 fromNif(const nif::compound::Matrix33 &m) {
  return Ogre::Matrix3{m.m11, m.m12, m.m13,
                       m.m21, m.m22, m.m23,
                       m.m31, m.m32, m.m33};
}

} // namespace engine::conversions

#endif //OPENOBLIVION_ENGINE_CONVERSIONS_HPP
