#ifndef OPENOBLIVION_NIF_COMPOUND_HPP
#define OPENOBLIVION_NIF_COMPOUND_HPP

#include "nif/basic.hpp"
#include "nif/enum.hpp"
#include "nif/versionable.hpp"
#include <array>
#include <vector>

namespace nif {
namespace compound {

using namespace nif::literals;

// BString
struct SizedString {
  basic::UInt length{};
  std::vector<basic::Char> value{};
};

struct String : Versionable {
  VersionOptional<SizedString, Unbounded, "20.0.0.5"_ver> string{version};
  VersionOptional<basic::StringIndex, "20.1.0.3"_ver, Unbounded> index{version};
  explicit String(uint32_t version) : Versionable(version) {}
};

struct ByteArray {
  basic::UInt dataSize{};
  std::vector<basic::Byte> data{};
};

struct ByteMatrix {
  basic::UInt dataSize1{};
  basic::UInt dataSize2{};
  // arr1 = dataSize2, arr2 = dataSize1
  std::vector<std::vector<basic::Byte>> data{};
};

template<class T>
struct Color3T {
  T r;
  T g;
  T b;
};

template<class T>
struct Color4T {
  T r;
  T g;
  T b;
  T a;
};

using Color3 = Color3T<basic::Float>;
using ByteColor3 = Color3T<basic::Byte>;
using Color4 = Color4T<basic::Float>;
using ByteColor4 = Color4T<basic::Byte>;

struct FilePath : Versionable {
  VersionOptional<SizedString, Unbounded, "20.0.0.5"_ver> string{version};
  VersionOptional<basic::StringIndex, "20.1.0.3"_ver, Unbounded> index{version};
  explicit FilePath(uint32_t version) : Versionable(version) {}
};

template<class NiObject>
struct Footer : Versionable {
  // Number of root references
  VersionOptional<basic::UInt, "3.3.0.13"_ver, Unbounded> numRoots{version};
  // List of root nif objects. If there is a camera for 1st person view then it
  // is included in this list even if it is not a root object.
  VersionOptional<std::vector<NiObject>, "3.3.0.13"_ver, Unbounded>
      roots{version};
  explicit Footer(uint32_t version) : Versionable(version) {}
};

struct LODRange {
  // Beginning of range
  basic::Float nearExtent{};
  // End of range
  basic::Float farExtent{};
  // Unknown
  std::array<basic::UInt, 3> unknown{};
};

struct MatchGroup {
  basic::UShort numVertices{};
  std::vector<basic::UShort> vertexIndices{};
};

template<class T>
struct Vector3T {
  T x;
  T y;
  T z;
};

template<class T>
struct Vector4T {
  T x;
  T y;
  T z;
  T w;
};

using ByteVector3 = Vector3T<basic::Byte>;
using Vector3 = Vector3T<basic::Float>;
using Vector4 = Vector4T<basic::Float>;

struct Quaternion {
  basic::Float w = 1.0f;
  basic::Float x = 0.0f;
  basic::Float y = 0.0f;
  basic::Float z = 0.0f;
};

struct hkQuaternion {
  basic::Float x = 0.0f;
  basic::Float y = 0.0f;
  basic::Float z = 0.0f;
  basic::Float w = 1.0f;
};

// Column major
struct Matrix22 {
  basic::Float m11 = 1.0f;
  basic::Float m21 = 0.0f;
  basic::Float m12 = 0.0f;
  basic::Float m22 = 1.0f;
};

// Column major
struct Matrix33 {
  basic::Float m11 = 1.0f;
  basic::Float m21 = 0.0f;
  basic::Float m31 = 0.0f;
  basic::Float m12 = 0.0f;
  basic::Float m22 = 1.0f;
  basic::Float m32 = 0.0f;
  basic::Float m13 = 0.0f;
  basic::Float m23 = 0.0f;
  basic::Float m33 = 1.0f;
};

// Column major
struct Matrix34 {
  basic::Float m11 = 1.0f;
  basic::Float m21 = 0.0f;
  basic::Float m31 = 0.0f;
  basic::Float m12 = 0.0f;
  basic::Float m22 = 1.0f;
  basic::Float m32 = 0.0f;
  basic::Float m13 = 0.0f;
  basic::Float m23 = 0.0f;
  basic::Float m33 = 1.0f;
  basic::Float m14 = 0.0f;
  basic::Float m24 = 0.0f;
  basic::Float m34 = 0.0f;
};

// Column major {
struct Matrix44 {
  basic::Float m11 = 1.0f;
  basic::Float m21 = 0.0f;
  basic::Float m31 = 0.0f;
  basic::Float m41 = 0.0f;
  basic::Float m12 = 0.0f;
  basic::Float m22 = 1.0f;
  basic::Float m32 = 0.0f;
  basic::Float m42 = 0.0f;
  basic::Float m13 = 0.0f;
  basic::Float m23 = 0.0f;
  basic::Float m33 = 1.0f;
  basic::Float m43 = 0.0f;
  basic::Float m14 = 0.0f;
  basic::Float m24 = 0.0f;
  basic::Float m34 = 0.0f;
  basic::Float m44 = 1.0f;
};

struct hkMatrix3 {
  basic::Float m11 = 1.0f;
  basic::Float m12 = 0.0f;
  basic::Float m13 = 0.0f;
  basic::Float m14{}; // Unused
  basic::Float m21 = 1.0f;
  basic::Float m22 = 0.0f;
  basic::Float m23 = 0.0f;
  basic::Float m24{}; // Unused
  basic::Float m31 = 1.0f;
  basic::Float m32 = 0.0f;
  basic::Float m33 = 0.0f;
  basic::Float m34{}; // Unused
};

struct MipMap {
  basic::UInt width{};
  basic::UInt height{};
  // Offset into the pixel data array where this mipmap starts.
  basic::UInt offset{};
};

template<class NiNode>
struct NodeSet {
  basic::UInt numNodes{};
  std::vector<basic::Ptr<NiNode>> nodes{};
};

// ver > 10.1.0.0
struct ShortString {
  basic::Byte length{};
  // Null-terminated string, length includes null-terminator
  std::vector<basic::Char> value;
};

struct ExportInfo {
  ShortString author{};
  ShortString processScript{};
  ShortString exportScript{};
};

struct Header : Versionable {
  // Should be 'NetImmerse File Format x.x.x.x' for ver < 10.0.1.2
  // Should be 'Gamebryo File Format x.x.x.x' for ver > 10.1.0.0
  basic::HeaderString headerString{};
  VersionOptional<std::array<basic::LineString, 3>, Unbounded, "3.1.0.0"_ver>
      copyright{version};
  VersionOptional<basic::FileVersion, "3.1.0.1"_ver, Unbounded> ver{version};
  VersionOptional<Enum::EndianType, "20.0.0.3"_ver, Unbounded>
      endianType{version, Enum::EndianType::ENDIAN_LITTLE};
  VersionOptional<basic::ULittle32, "10.0.1.8"_ver, Unbounded> userVer{version};
  // Number of file objects
  VersionOptional<basic::ULittle32, "3.1.0.1"_ver, Unbounded>
      numBlocks{version};

  struct BSStreamHeader {
    // ver == 10.0.1.2 ||
    //  (ver == 20.2.0.7 || ver = 20.0.0.5 ||
    //    (ver >= 10.1.0.0 && ver <= 20.0.0.4 && userver <= 11))
    // && userver >= 3
    basic::ULittle32 userVersion2{};
    // ver == 10.0.1.2 ||
    //  (ver == 20.2.0.7 || ver == 20.0.0.5 ||
    //    (ver >= 10.1.0.0 && ver <= 20.0.0.4 && userver <= 11)))
    // && userver >= 3
    ExportInfo exportInfo{};
    // userVersion2 == 130
    ShortString maxFilepath;
  };
  BSStreamHeader bsStreamHeader;

  explicit Header(uint32_t version) : Versionable(version) {}
};

} // namespace compound
} // namespace nif

#endif // OPENOBLIVION_NIF_COMPOUND_HPP
