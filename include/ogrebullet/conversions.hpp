#ifndef OPENOBLIVION_OGREBULLET_CONVERSIONS_HPP
#define OPENOBLIVION_OGREBULLET_CONVERSIONS_HPP

#include <btBulletDynamicsCommon.h>
#include <Ogre.h>

namespace Ogre::conversions {

inline btVector3 toBullet(const Ogre::Vector3 &v) {
  return btVector3{v.x, v.y, v.z};
}

inline btQuaternion toBullet(const Ogre::Quaternion &q) {
  return btQuaternion{q.x, q.y, q.z, q.w};
}

inline btMatrix3x3 toBullet(const Ogre::Matrix3 &m) {
  return btMatrix3x3{m[0][0], m[0][1], m[0][2],
                     m[1][0], m[1][1], m[1][2],
                     m[2][0], m[2][1], m[2][2]};
}

inline Ogre::Vector3 fromBullet(const btVector3 &v) {
  return Ogre::Vector3{v.x(), v.y(), v.z()};
}

inline Ogre::Quaternion fromBullet(const btQuaternion &q) {
  return Ogre::Quaternion{q.w(), q.x(), q.y(), q.z()};
}

} // namespace Ogre::conversions

#endif // OPENOBLIVION_OGREBULLET_CONVERSIONS_HPP
