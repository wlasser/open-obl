#ifndef OPENOBLIVION_CONVERSIONS_HPP
#define OPENOBLIVION_CONVERSIONS_HPP

#include "nif/compound.hpp"
#include <boost/qvm/all.hpp>
#include <btBulletDynamicsCommon.h>
#include <OgreColourValue.h>
#include <OgreMatrix3.h>
#include <OgreMatrix4.h>
#include <OgreVector.h>
#include <filesystem>

namespace boost::qvm {

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
  read_element(Ogre::Vector4 &v) {
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

} // namespace boost::qvm

namespace oo {

namespace qvm = boost::qvm;

// Game data uses 'u' has a unit of distance, with 64 u = 1 yd, but Bullet works
// best with (needs?) SI units. By definition, 1 yd = 0.9144 m.
template<class T> constexpr T unitsPerMeter = T(64.0L / 0.9144L);
template<class T> constexpr T metersPerUnit = T(0.9144L / 64.0L);

inline Ogre::ColourValue fromNif(const nif::compound::Color3 &c) {
  return Ogre::ColourValue(c.r, c.g, c.b);
}

inline Ogre::ColourValue fromNif(const nif::compound::Color4 &c) {
  return Ogre::ColourValue(c.r, c.g, c.b, c.a);
}

template<class Vec, typename = std::enable_if_t<
    qvm::is_vec<Vec>::value && qvm::vec_traits<Vec>::dim == 3>>
Ogre::Vector3 fromBSCoordinates(const Vec &v) {
  using namespace qvm;
  return Ogre::Vector3{qvm::X(v), qvm::Z(v), -qvm::Y(v)} *
      oo::metersPerUnit<typename qvm::vec_traits<Vec>::scalar_type>;
}

template<class Vec, typename = std::enable_if_t<
    qvm::is_vec<Vec>::value && qvm::vec_traits<Vec>::dim == 4>>
Ogre::Vector4 fromBSCoordinates(const Vec &v) {
  return Ogre::Vector4(oo::fromBSCoordinates(qvm::XYZ(v)), qvm::W(v));
}

template<class Mat, typename = std::enable_if_t<
    qvm::is_mat<Mat>::value
        && qvm::mat_traits<Mat>::rows == 3
        && qvm::mat_traits<Mat>::cols == 3>>
Ogre::Matrix3 fromBSCoordinates(const Mat &m) {
  using namespace qvm;
  const auto &C{qvm::rotx_mat<3>(-Ogre::Math::HALF_PI)};
  const auto &CInv{qvm::transposed(C)};
  return C * m * CInv;
}

template<class Mat, typename = std::enable_if_t<
    qvm::is_mat<Mat>::value
        && qvm::mat_traits<Mat>::rows == 4
        && qvm::mat_traits<Mat>::cols == 4>>
Ogre::Matrix4 fromBSCoordinates(const Mat &m) {
  using namespace qvm;
  const auto k{oo::metersPerUnit<typename qvm::mat_traits<Mat>::scalar_type>};
  const auto &S{qvm::diag_mat(qvm::XXX1(k))};
  const auto &C{qvm::rotx_mat<4>(-Ogre::Math::HALF_PI) * S};

  const auto &SInv{qvm::diag_mat(qvm::XXX1(1.0 / k))};
  const auto &CInv{qvm::rotx_mat<4>(Ogre::Math::HALF_PI) * SInv};

  return C * m * CInv;
}

template<class Quat, typename = std::enable_if_t<qvm::is_quat<Quat>::value>>
Ogre::Quaternion fromBSCoordinates(const Quat &q) {
  using namespace qvm;
  const auto &p{qvm::rotx_quat(-Ogre::Math::HALF_PI)};
  const auto &pInv{qvm::rotx_quat(Ogre::Math::HALF_PI)};
  return p * q * pInv;
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

} // namespace oo

#endif // OPENOBLIVION_CONVERSIONS_HPP
