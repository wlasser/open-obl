#ifndef OPENOBLIVION_ENGINE_CONVERSIONS_HPP
#define OPENOBLIVION_ENGINE_CONVERSIONS_HPP

#include "nif/compound.hpp"
#include <btBulletDynamicsCommon.h>
#include <OgreColourValue.h>
#include <OgreMatrix3.h>
#include <OgreVector.h>
#include <filesystem>

namespace engine::conversions {

// Game data uses 'u' has a unit of distance, with 64 u = 1 yd, but Bullet works
// best with (needs?) SI units. By definition, 1 yd = 0.9144 m.
template<class T>
constexpr T unitsPerMeter = T(64.0L / 0.9144L);
template<class T>
constexpr T metersPerUnit = T(0.9144L / 64.0L);

inline Ogre::Vector3 fromNif(const nif::compound::Vector3 &v) {
  return Ogre::Vector3{v.x, v.y, v.z};
}

inline Ogre::Vector4 fromNif(const nif::compound::Vector4 &v) {
  return Ogre::Vector4{v.x, v.y, v.z, v.w};
}

inline Ogre::Quaternion fromNif(const nif::compound::hkQuaternion &q) {
  return Ogre::Quaternion{q.w, q.x, q.y, q.z};
}

inline Ogre::Matrix3 fromNif(const nif::compound::Matrix33 &m) {
  return Ogre::Matrix3{m.m11, m.m12, m.m13,
                       m.m21, m.m22, m.m23,
                       m.m31, m.m32, m.m33};
}

inline Ogre::ColourValue fromNif(const nif::compound::Color3 &c) {
  return Ogre::ColourValue(c.r, c.g, c.b);
}

inline Ogre::ColourValue fromNif(const nif::compound::Color4 &c) {
  return Ogre::ColourValue(c.r, c.g, c.b, c.a);
}

inline Ogre::Vector3 fromBSCoordinates(const Ogre::Vector3 &v) {
  return metersPerUnit<Ogre::Real> * Ogre::Vector3{v.x, v.z, -v.y};
}

inline Ogre::Matrix3 fromBSCoordinates(const Ogre::Matrix3 &m) {
  const Ogre::Matrix3 C = {1, 0, 0,
                           0, 0, 1,
                           0, -1, 0};
  return C * m * C.transpose();
}

inline Ogre::Quaternion fromBSCoordinates(const Ogre::Quaternion &m) {
  // TODO: This is needlessly inefficient
  Ogre::Matrix3 mat{Ogre::Matrix3::IDENTITY};
  m.ToRotationMatrix(mat);
  return Ogre::Quaternion{fromBSCoordinates(mat)};
}

// Convert a windows path to a lowercase nix path
inline std::string normalizePath(std::string path) {
  std::string out(path);
  std::transform(path.begin(), path.end(), out.begin(),
                 [](unsigned char c) {
                   return std::tolower(c == '\\' ? '/' : c);
                 });
  return out;
}

} // namespace engine::conversions

#endif //OPENOBLIVION_ENGINE_CONVERSIONS_HPP
