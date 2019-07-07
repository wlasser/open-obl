#ifndef OPENOBL_CONVERSIONS_HPP
#define OPENOBL_CONVERSIONS_HPP

#include "nif/compound.hpp"
#include <boost/qvm/all.hpp>
#include <btBulletDynamicsCommon.h>
#include <OgreColourValue.h>
#include <OgreMatrix3.h>
#include <OgreMatrix4.h>
#include <OgreVector.h>
#include "util/windows_cleanup.hpp"

/// \defgroup OpenOBLMath Math Compatibility Library
/// Provides additional math functions and conversions between different
/// components' linear algebra types.

/// \file
/// \ingroup OpenOBLMath
/// Functions to convert between different coordinate systems and linear
/// algebra types.
///
/// Different parts of the game engine are required---for various reasons---to
/// use different coordinate systems and linear algebra types. Converting
/// between the types is achieved through a mix of conversion factor variable
/// templates, for changing units; Boost QVM specializations, for changing
/// linear algebra types; and function templates, for changing coordinate
/// systems.
///
/// The preferred coordinate system used by the engine is called the *Ogre* or
/// *Bullet* coordinate system. (Not because those engines force it upon us,
/// just because it's what we use with those engines). SI units are used to
/// measure masses, distances, velocities and so on; a mass of `1.0f`
/// corresponds to `1 kg`, a distance of `1.0f` corresponds to `1 m`, etc.
/// Regarding axes, the \f$x\f$-axis increases eastwards, the \f$z\f$-axis
/// increases southwards, and the \f$y\f$-axis increases 'upwards', i.e.
/// opposite to the direction of gravity and such that the \f$(x,y,z)\f$
/// coordinates form a right-handed coordinate system. In terrible ASCII art,
///
/// ```
///    Y+
///    |
///    |
///    |_____X+ (East)
///   /
///  /
/// Z+ (South)
/// ```
///
/// A positive rotation about an axis in the Ogre coordinate system corresponds
/// to a counter-clockwise rotation about that axis, as in the right-hand rule.
///
/// The *BS* coordinate system used by the game data differs from the Ogre
/// coordinate system. Distances are measured in (what we call) 'BS units',
/// though masses are still measured in kilograms. The \f$x\f$-axis increases
/// eastwards, the \f$y\f$-axis increases northwards, and the \f$z\f$-axis
/// increases upwards, again in the opposite direction to gravity and such that
/// \f$(x,y,z)\f$ forms a right-handed coordinate system. In a picture,
///
/// ```
/// Z+
/// |  Y+ (North)
/// | /
/// |/_____X+ (East)
/// ```
///
/// One sees that the axes of the Ogre coordinate system are obtained from the
/// BS coordinate system by a \f$90\f$ degree counter-clockwise rotation about
/// the \f$x\f$-axis.

//===----------------------------------------------------------------------===//
// Boost QVM Specializations
//===----------------------------------------------------------------------===//
namespace boost::qvm {

/// \name Boost QVM Specializations
/// @{

template<> struct vec_traits<nif::compound::Vector3> {
  static const int dim{3};
  using scalar_type = nif::basic::Float;

  template<int I> static inline scalar_type &
  write_element(nif::compound::Vector3 &v) {
    if constexpr (I == 0) return v.x;
    else if constexpr (I == 1) return v.y;
    else return v.z;
  }

  template<int I> static inline scalar_type
  read_element(const nif::compound::Vector3 &v) {
    if constexpr (I == 0) return v.x;
    else if constexpr (I == 1) return v.y;
    else return v.z;
  }
};

template<> struct vec_traits<Ogre::Vector3> {
  static const int dim{3};
  using scalar_type = Ogre::Real;

  template<int I> static inline scalar_type &
  write_element(Ogre::Vector3 &v) {
    if constexpr (I == 0) return v.x;
    else if constexpr (I == 1) return v.y;
    else return v.z;
  }

  template<int I> static inline scalar_type
  read_element(const Ogre::Vector3 &v) {
    if constexpr (I == 0) return v.x;
    else if constexpr (I == 1) return v.y;
    else return v.z;
  }
};

template<> struct vec_traits<btVector3> {
  static const int dim{3};
  using scalar_type = btScalar;

  template<int I> static inline scalar_type &
  write_element(btVector3 &v) {
    if constexpr (I == 0) return v[0];
    else if constexpr (I == 1) return v[1];
    else return v[2];
  }

  template<int I> static inline scalar_type
  read_element(const btVector3 &v) {
    if constexpr (I == 0) return v[0];
    else if constexpr (I == 1) return v[1];
    else return v[2];
  }
};

template<> struct vec_traits<nif::compound::Vector4> {
  static const int dim{4};
  using scalar_type = nif::basic::Float;

  template<int I> static inline scalar_type &
  write_element(nif::compound::Vector4 &v) {
    if constexpr (I == 0) return v.x;
    else if constexpr (I == 1) return v.y;
    else if constexpr (I == 2) return v.z;
    else return v.w;
  }

  template<int I> static inline scalar_type
  read_element(const nif::compound::Vector4 &v) {
    if constexpr (I == 0) return v.x;
    else if constexpr (I == 1) return v.y;
    else if constexpr (I == 2) return v.z;
    else return v.w;
  }
};

template<> struct vec_traits<Ogre::Vector4> {
  static const int dim{4};
  using scalar_type = Ogre::Real;

  template<int I> static inline scalar_type &
  write_element(Ogre::Vector4 &v) {
    if constexpr (I == 0) return v.x;
    else if constexpr (I == 1) return v.y;
    else if constexpr (I == 2) return v.z;
    else return v.w;
  }

  template<int I> static inline scalar_type
  read_element(const Ogre::Vector4 &v) {
    if constexpr (I == 0) return v.x;
    else if constexpr (I == 1) return v.y;
    else if constexpr (I == 2) return v.z;
    else return v.w;
  }
};

template<> struct quat_traits<nif::compound::Quaternion> {
  using scalar_type = nif::basic::Float;

  template<int I> static inline scalar_type &
  write_element(nif::compound::Quaternion &q) {
    if constexpr (I == 0) return q.w;
    else if constexpr (I == 1) return q.x;
    else if constexpr (I == 2) return q.y;
    else return q.z;
  }

  template<int I> static inline scalar_type
  read_element(const nif::compound::Quaternion &q) {
    if constexpr (I == 0) return q.w;
    else if constexpr (I == 1) return q.x;
    else if constexpr (I == 2) return q.y;
    else return q.z;
  }
};

template<> struct quat_traits<nif::compound::hk::Quaternion> {
  using scalar_type = nif::basic::Float;

  template<int I> static inline scalar_type &
  write_element(nif::compound::hk::Quaternion &q) {
    if constexpr (I == 0) return q.w;
    else if constexpr (I == 1) return q.x;
    else if constexpr (I == 2) return q.y;
    else return q.z;
  }

  template<int I> static inline scalar_type
  read_element(const nif::compound::hk::Quaternion &q) {
    if constexpr (I == 0) return q.w;
    else if constexpr (I == 1) return q.x;
    else if constexpr (I == 2) return q.y;
    else return q.z;
  }
};

template<> struct quat_traits<Ogre::Quaternion> {
  using scalar_type = Ogre::Real;

  template<int I> static inline scalar_type &
  write_element(Ogre::Quaternion &q) {
    if constexpr (I == 0) return q.w;
    else if constexpr (I == 1) return q.x;
    else if constexpr (I == 2) return q.y;
    else return q.z;
  }

  template<int I> static inline scalar_type
  read_element(const Ogre::Quaternion &q) {
    if constexpr (I == 0) return q.w;
    else if constexpr (I == 1) return q.x;
    else if constexpr (I == 2) return q.y;
    else return q.z;
  }
};

template<> struct quat_traits<btQuaternion> {
  using scalar_type = btScalar;

  template<int I> static inline scalar_type &
  write_element(btQuaternion &q) {
    if constexpr (I == 0) return q[0];
    else if constexpr (I == 1) return q[1];
    else if constexpr (I == 2) return q[2];
    else return q[3];
  }

  template<int I> static inline scalar_type
  read_element(const btQuaternion &q) {
    if constexpr (I == 0) return q[0];
    else if constexpr (I == 1) return q[1];
    else if constexpr (I == 2) return q[2];
    else return q[3];
  }
};

template<> struct mat_traits<nif::compound::Matrix33> {
  static const int rows{3};
  static const int cols{3};
  using scalar_type = nif::basic::Float;

  template<int R, int C> static inline scalar_type &
  write_element(nif::compound::Matrix33 &m) {
    if constexpr (R == 0) {
      if constexpr (C == 0) return m.m11;
      else if constexpr (C == 1) return m.m12;
      else return m.m13;
    } else if constexpr (R == 1) {
      if constexpr (C == 0) return m.m21;
      else if constexpr (C == 1) return m.m22;
      else return m.m23;
    } else {
      if constexpr (C == 0) return m.m31;
      else if constexpr (C == 1) return m.m32;
      else return m.m33;
    }
  }

  template<int R, int C> static inline scalar_type
  read_element(const nif::compound::Matrix33 &m) {
    if constexpr (R == 0) {
      if constexpr (C == 0) return m.m11;
      else if constexpr (C == 1) return m.m12;
      else return m.m13;
    } else if constexpr (R == 1) {
      if constexpr (C == 0) return m.m21;
      else if constexpr (C == 1) return m.m22;
      else return m.m23;
    } else {
      if constexpr (C == 0) return m.m31;
      else if constexpr (C == 1) return m.m32;
      else return m.m33;
    }
  }
};

template<> struct mat_traits<Ogre::Matrix3> {
  static const int rows{3};
  static const int cols{3};
  using scalar_type = Ogre::Real;

  template<int R, int C> static inline scalar_type &
  write_element(Ogre::Matrix3 &m) {
    return m[R][C];
  }

  template<int R, int C> static inline scalar_type
  read_element(const Ogre::Matrix3 &m) {
    return m[R][C];
  }
};

template<> struct mat_traits<btMatrix3x3> {
  static const int rows{3};
  static const int cols{3};
  using scalar_type = btScalar;

  template<int R, int C> static inline scalar_type &
  write_element(btMatrix3x3 &m) {
    return m[R][C];
  }

  template<int R, int C> static inline scalar_type
  read_element(const btMatrix3x3 &m) {
    return m[R][C];
  }
};

template<> struct mat_traits<nif::compound::Matrix44> {
  static const int rows{4};
  static const int cols{4};
  using scalar_type = nif::basic::Float;

  template<int R, int C> static inline scalar_type &
  write_element(nif::compound::Matrix44 &m) {
    if constexpr (R == 0) {
      if constexpr (C == 0) return m.m11;
      else if constexpr (C == 1) return m.m12;
      else if constexpr (C == 2) return m.m13;
      else return m.m14;
    } else if constexpr (R == 1) {
      if constexpr (C == 0) return m.m21;
      else if constexpr (C == 1) return m.m22;
      else if constexpr (C == 2) return m.m23;
      else return m.m24;
    } else if constexpr (R == 2) {
      if constexpr (C == 0) return m.m31;
      else if constexpr (C == 1) return m.m32;
      else if constexpr (C == 2) return m.m33;
      else return m.m34;
    } else {
      if constexpr (C == 0) return m.m41;
      else if constexpr (C == 1) return m.m42;
      else if constexpr (C == 2) return m.m43;
      else return m.m44;
    }
  }

  template<int R, int C> static inline scalar_type
  read_element(const nif::compound::Matrix44 &m) {
    if constexpr (R == 0) {
      if constexpr (C == 0) return m.m11;
      else if constexpr (C == 1) return m.m12;
      else if constexpr (C == 2) return m.m13;
      else return m.m14;
    } else if constexpr (R == 1) {
      if constexpr (C == 0) return m.m21;
      else if constexpr (C == 1) return m.m22;
      else if constexpr (C == 2) return m.m23;
      else return m.m24;
    } else if constexpr (R == 2) {
      if constexpr (C == 0) return m.m31;
      else if constexpr (C == 1) return m.m32;
      else if constexpr (C == 2) return m.m33;
      else return m.m34;
    } else {
      if constexpr (C == 0) return m.m41;
      else if constexpr (C == 1) return m.m42;
      else if constexpr (C == 2) return m.m43;
      else return m.m44;
    }
  }
};

template<> struct mat_traits<Ogre::Matrix4> {
  static const int rows{4};
  static const int cols{4};
  using scalar_type = Ogre::Real;

  template<int R, int C> static inline scalar_type &
  write_element(Ogre::Matrix4 &m) {
    return m[R][C];
  }

  template<int R, int C> static inline scalar_type
  read_element(const Ogre::Matrix4 &m) {
    return m[R][C];
  }
};

///@}

} // namespace boost::qvm

// These are necessary to make swizzling work without explicit convert_to calls.
namespace Ogre {
using boost::qvm::assign;
}
using boost::qvm::assign;

namespace oo {

namespace qvm = boost::qvm;

using namespace qvm::sfinae;

//===----------------------------------------------------------------------===//
// Conversion factors
//===----------------------------------------------------------------------===//

/// The number of BS units in a meter.
/// The game data uses `u` as a unit of distance, with `64 u = 1 yd`, but
/// Bullet works best with SI units. By definition, `1 yd = 0.9144 m`.
template<class T> constexpr T unitsPerMeter = T(64.0L / 0.9144L);
/// The number of meters in a BS unit.
template<class T> constexpr T metersPerUnit = T(0.9144L / 64.0L);

/// The length of an edge of an exterior cell in BS units.
/// Exterior cells are square.
template<class T> constexpr T unitsPerCell = T(4096.0L);
/// The length of an edge of a cell quadrant in BS units.
/// Exterior cells are split into four square quadrants for texturing purposes.
template<class T> constexpr T unitsPerQuad = unitsPerCell<T> / T(2);
/// The number of vertices along the edge of an exterior cell, as a closed
/// range. Must be one plus a power of two.
template<class T> constexpr T verticesPerCell = T(33u);
/// The number of vertices along the edge of an exterior cell quad, as a closed
/// range. This could be worked out from `oo::verticesPerCell`.
template<class T> constexpr T verticesPerQuad = T(17u);
/// The length of an edge of a distant cell chunk in BS units.
/// Distant cell chunks are square.
template<class T> constexpr T unitsPerChunk = unitsPerCell<T> * T(32.0f);

/// The number of Havok units in a BS unit.
/// Havok uses units 'hu' such that `7u = 1hu`.
template<class T> constexpr T havokUnitsPerUnit = T(1.0L / 7.0L);
/// The number of BS units in a Havok unit.
template<class T> constexpr T unitsPerHavokUnit = T(7.0L);

//===----------------------------------------------------------------------===//
// Conversion factors
//===----------------------------------------------------------------------===//

/// Colour Conversions
/// @{
inline Ogre::ColourValue fromNif(const nif::compound::Color3 &c) {
  return Ogre::ColourValue(c.r, c.g, c.b);
}

inline Ogre::ColourValue fromNif(const nif::compound::Color4 &c) {
  return Ogre::ColourValue(c.r, c.g, c.b, c.a);
}
/// @}

/// Convert a QVM-compatible vector from BS coordinates into Ogre coordinates.
template<class Vec, typename = std::enable_if_t<
    qvm::is_vec<Vec>::value && qvm::vec_traits<Vec>::dim == 3>>
Ogre::Vector3 fromBSCoordinates(const Vec &v) {
  return Ogre::Vector3{qvm::X(v), qvm::Z(v), -qvm::Y(v)} *
      oo::metersPerUnit<typename qvm::vec_traits<Vec>::scalar_type>;
}

/// Convert a QVM-comaptible vector from Ogre coordinates into BS coordinates.
template<class Vec, typename = std::enable_if_t<
    qvm::is_vec<Vec>::value && qvm::vec_traits<Vec>::dim == 3>>
Ogre::Vector3 toBSCoordinates(const Vec &v) {
  return Ogre::Vector3{qvm::X(v), -qvm::Z(v), qvm::Y(v)} *
      oo::unitsPerMeter<typename qvm::vec_traits<Vec>::scalar_type>;
}

/// Convert a QVM-compatible vector from BS coordinates into Ogre coordinates.
template<class Vec, typename = std::enable_if_t<
    qvm::is_vec<Vec>::value && qvm::vec_traits<Vec>::dim == 4>>
Ogre::Vector4 fromBSCoordinates(const Vec &v) {
  return Ogre::Vector4(oo::fromBSCoordinates(qvm::XYZ(v)), qvm::W(v));
}

/// Convert a QVM-compatible vector from Ogre coordinates into BS coordinates.
template<class Vec, typename = std::enable_if_t<
    qvm::is_vec<Vec>::value && qvm::vec_traits<Vec>::dim == 4>>
Ogre::Vector4 toBSCoordinates(const Vec &v) {
  return Ogre::Vector4(oo::toBSCoordinates(qvm::XYZ(v)), qvm::W(v));
}

/// Convert a QVM-compatible transformation matrix from BS coordinates into
/// Ogre coordinates.
template<class Mat, typename = std::enable_if_t<
    qvm::is_mat<Mat>::value
        && qvm::mat_traits<Mat>::rows == 3
        && qvm::mat_traits<Mat>::cols == 3>>
Ogre::Matrix3 fromBSCoordinates(const Mat &m) {
  const Ogre::Matrix3 C = qvm::rotx_mat<3>(-Ogre::Math::HALF_PI);
  const auto &CInv{qvm::transposed(C)};
  return C * m * CInv;
}

/// Convert a QVM-compatible transformation matrix from Ogre coordinates into
/// BS coordinates.
template<class Mat, typename = std::enable_if_t<
    qvm::is_mat<Mat>::value
        && qvm::mat_traits<Mat>::rows == 3
        && qvm::mat_traits<Mat>::cols == 3>>
Ogre::Matrix3 toBSCoordinates(const Mat &m) {
  const Ogre::Matrix3 C = qvm::rotx_mat<3>(-Ogre::Math::HALF_PI);
  const auto &CInv{qvm::transposed(C)};
  return CInv * m * C;
}

/// Convert a QVM-compatible transformation matrix from BS coordinates into
/// Ogre coordinates.
template<class Mat, typename = std::enable_if_t<
    qvm::is_mat<Mat>::value
        && qvm::mat_traits<Mat>::rows == 4
        && qvm::mat_traits<Mat>::cols == 4>>
Ogre::Matrix4 fromBSCoordinates(const Mat &m) {
  const auto k{oo::metersPerUnit<typename qvm::mat_traits<Mat>::scalar_type>};
  const auto &S{qvm::diag_mat(qvm::XXX1(k))};
  const Ogre::Matrix4 C = qvm::rotx_mat<4>(-Ogre::Math::HALF_PI) * S;

  const auto &SInv{qvm::diag_mat(qvm::XXX1(1.0 / k))};
  const Ogre::Matrix4 CInv = qvm::rotx_mat<4>(Ogre::Math::HALF_PI) * SInv;

  return C * m * CInv;
}

/// Convert a QVM-compatible transformation matrix from Ogre coordinates into
/// BS coordinates.
template<class Mat, typename = std::enable_if_t<
    qvm::is_mat<Mat>::value
        && qvm::mat_traits<Mat>::rows == 4
        && qvm::mat_traits<Mat>::cols == 4>>
Ogre::Matrix4 toBSCoordinates(const Mat &m) {
  const auto k{oo::metersPerUnit<typename qvm::mat_traits<Mat>::scalar_type>};
  const auto &S{qvm::diag_mat(qvm::XXX1(k))};
  const Ogre::Matrix4 C = qvm::rotx_mat<4>(-Ogre::Math::HALF_PI) * S;

  const auto &SInv{qvm::diag_mat(qvm::XXX1(1.0 / k))};
  const Ogre::Matrix4 CInv = qvm::rotx_mat<4>(Ogre::Math::HALF_PI) * SInv;

  return CInv * m * C;
}

/// Convert a QVM-compatible quaternion from BS coordinates to Ogre
/// coordinates.
template<class Quat, typename = std::enable_if_t<qvm::is_quat<Quat>::value>>
Ogre::Quaternion fromBSCoordinates(const Quat &q) {
  const Ogre::Quaternion p = qvm::rotx_quat(-Ogre::Math::HALF_PI);
  const Ogre::Quaternion pInv = qvm::rotx_quat(Ogre::Math::HALF_PI);
  return p * q * pInv;
}

/// Convert a QVM-compatible quaternion from Ogre coordinates to BS coordinates.
template<class Quat, typename = std::enable_if_t<qvm::is_quat<Quat>::value>>
Ogre::Quaternion toBSCoordinates(const Quat &q) {
  const Ogre::Quaternion p = qvm::rotx_quat(-Ogre::Math::HALF_PI);
  const Ogre::Quaternion pInv = qvm::rotx_quat(Ogre::Math::HALF_PI);
  return pInv * q * p;
}

/// Convert a QVM-compatible vector from Havok coordinates to Ogre coordinates.
template<class Vec, typename = std::enable_if_t<
    qvm::is_vec<Vec>::value && qvm::vec_traits<Vec>::dim == 3>>
Ogre::Vector3 fromHavokCoordinates(const Vec &v) {
  using scalar_type = typename qvm::vec_traits<Vec>::scalar_type;
  return Ogre::Vector3{qvm::X(v), qvm::Z(v), -qvm::Y(v)} *
      oo::metersPerUnit<scalar_type> * oo::unitsPerHavokUnit<scalar_type>;
}

/// Convert a QVM-compatible vector from Havok coordinates to Ogre coordinates.
template<class Vec, typename = std::enable_if_t<
    qvm::is_vec<Vec>::value && qvm::vec_traits<Vec>::dim == 4>>
Ogre::Vector4 fromHavokCoordinates(const Vec &v) {
  return Ogre::Vector4(oo::fromHavokCoordinates(qvm::XYZ(v)), qvm::W(v));
}

/// Convert a QVM-compatible transformation matrix from Havok coordinates to
/// Ogre coordinates.
template<class Mat, typename = std::enable_if_t<
    qvm::is_mat<Mat>::value
        && qvm::mat_traits<Mat>::rows == 3
        && qvm::mat_traits<Mat>::cols == 3>>
Ogre::Matrix3 fromHavokCoordinates(const Mat &m) {
  return oo::fromBSCoordinates(m);
}

/// Convert a QVM-compatible transformation matrix from Havok coordinates to
/// Ogre coordinates.
template<class Mat, typename = std::enable_if_t<
    qvm::is_mat<Mat>::value
        && qvm::mat_traits<Mat>::rows == 4
        && qvm::mat_traits<Mat>::cols == 4>>
Ogre::Matrix4 fromHavokCoordinates(const Mat &m) {
  using scalar_type = typename qvm::mat_traits<Mat>::scalar_type;
  const auto k{oo::metersPerUnit<scalar_type> *
      oo::unitsPerHavokUnit<scalar_type>};
  const auto &S{qvm::diag_mat(qvm::XXX1(k))};
  const Ogre::Matrix4 C = qvm::rotx_mat<4>(-Ogre::Math::HALF_PI) * S;

  const auto &SInv{qvm::diag_mat(qvm::XXX1(1.0 / k))};
  const Ogre::Matrix4 CInv = qvm::rotx_mat<4>(Ogre::Math::HALF_PI) * SInv;

  return C * m * CInv;
}

/// Convert a QVM-compatible quaternion from Havok coordinates to Ogre
/// coordinates.
template<class Quat, typename = std::enable_if_t<qvm::is_quat<Quat>::value>>
Ogre::Quaternion fromHavokCoordinates(const Quat &q) {
  return oo::fromBSCoordinates(q);
}

/// Convert a `z-y-x` Tait-Bryan angle rotation into a quaternion.
/// The rotations are extrinsic rotations in the BS coordinate system, with
/// position rotations referring to *clockwise* rotations, not anticlockwise.
/// This is to be consistent with `record::raw::REFRTransformation`.
inline Ogre::Quaternion
fromBSTaitBryan(Ogre::Radian aX, Ogre::Radian aY, Ogre::Radian aZ) {
  // This can no doubt be optimized by constructing a quaternion directly from
  // the angle data, building in the coordinate change, but building a rotation
  // matrix and changing coordinates was conceptually simpler.
  Ogre::Matrix3 rotX, rotY, rotZ;
  rotX.FromAngleAxis(Ogre::Vector3::UNIT_X, Ogre::Radian(-aX));
  rotY.FromAngleAxis(Ogre::Vector3::UNIT_Y, Ogre::Radian(-aY));
  rotZ.FromAngleAxis(Ogre::Vector3::UNIT_Z, Ogre::Radian(-aZ));
  const auto rotMat{oo::fromBSCoordinates(rotX * rotY * rotZ)};
  return Ogre::Quaternion(rotMat);
}

} // namespace oo

#endif // OPENOBL_CONVERSIONS_HPP
