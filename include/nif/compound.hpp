#ifndef OPENOBLIVION_NIF_COMPOUND_HPP
#define OPENOBLIVION_NIF_COMPOUND_HPP

#include "io/read_bytes.hpp"
#include "nif/basic.hpp"
#include "nif/enum.hpp"
#include "nif/versionable.hpp"
#include <array>
#include <variant>
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

// TODO: This looks suspicious; is roots really a homogeneous array of actual
// NiObject types, or is it an array of objects deriving from NiObject?
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

// TODO: Not sure if pointers to concrete NiNode, or polymorphic
template<class NiNode>
struct NodeSet {
  basic::UInt numNodes{};
  std::vector<basic::Ptr<NiNode>> nodes{};
};

// ver > 10.1.0.0
struct ShortString {
  basic::Byte length{};
  // Null-terminated string, length includes null-terminator
  std::vector<basic::Char> value{};
};

struct ExportInfo {
  ShortString author{};
  ShortString processScript{};
  ShortString exportScript{};
};

struct NiPlane {
  Vector3 normal;
  basic::Float constant;
};

struct NiBound {
  Vector3 center;
  basic::Float radius;
};

struct BoxBV {
  Vector3 center{};
  std::array<Vector3, 3> axis{};
  Vector3 extent{};
};

struct CapsuleBV {
  Vector3 center{};
  Vector3 origin{};
  basic::Float extent{};
  basic::Float radius{};
};

struct BoundingVolume;

struct UnionBV {
  basic::UInt numBoundingVolumes;
  std::vector<BoundingVolume> boundingVolumes;
};

struct HalfSpaceBV {
  NiPlane plane{};
  Vector3 center{};
};

struct EmptyBV {};
struct BoundingVolume {
  Enum::BoundVolumeType collisionType{};
  std::variant<NiBound, BoxBV, CapsuleBV, EmptyBV, UnionBV, HalfSpaceBV>
      collision{};
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
    ShortString maxFilepath{};
  };
  BSStreamHeader bsStreamHeader{};

  VersionOptional<ByteArray, "30.0.0.0"_ver, Unbounded> metadata{version};

  // Number of object types
  VersionOptional<basic::UShort, "5.0.0.1"_ver, Unbounded>
      numBlockTypes{version};

  // List of object types
  VersionOptional<std::vector<SizedString>, "5.0.0.1"_ver, Unbounded>
      blockTypes{version};

  // Map of objects into object types; i-th entry is the index in blockTypes
  // corresponding to the type of the i-th object.
  VersionOptional<std::vector<basic::BlockTypeIndex>, "5.0.0.1"_ver, Unbounded>
      blockTypeIndices{version};

  VersionOptional<std::vector<basic::UInt>, "20.2.0.5"_ver, Unbounded>
      blockSizes{version};

  VersionOptional<basic::UInt, "20.1.0.1"_ver, Unbounded> numStrings{version};

  VersionOptional<basic::UInt, "20.1.0.1"_ver, Unbounded>
      maxStringLength{version};

  VersionOptional<std::vector<SizedString>, "20.1.0.1"_ver, Unbounded>
      strings{version};

  VersionOptional<basic::UInt, "5.0.0.6"_ver, Unbounded> numGroups{version};

  VersionOptional<std::vector<basic::UInt>, "5.0.0.6"_ver, Unbounded>
      groups{version};

  explicit Header(uint32_t version) : Versionable(version) {}
};

struct MaterialData {
  basic::Bool hasShader{};
  std::string shaderName{};
  // -1 means default shader implementation
  basic::Int shaderExtraData{};
};

struct Triangle {
  // 3 vertex indices
  basic::UShort v1;
  basic::UShort v2;
  basic::UShort v3;
};

// ver > 4.2.1.0
struct SkinPartition : Versionable {
  basic::UShort numVertices{};
  // Calculated
  basic::UShort numTriangles{};
  basic::UShort numBones{};
  // 0 if submesh is not stripped
  basic::UShort numStrips{};
  basic::UShort numWeightsPerVertex{};
  std::vector<basic::UShort> bones{};

  VersionOptional<basic::Bool, "10.1.0.0"_ver, Unbounded> hasVertexMap{version};
  // Maps the weight/influence lists in this submesh to the vertices in the
  // shape being skinned. Vertex map was compulsory before 10.1.0.0.
  std::vector<basic::UShort> vertexMap{};

  VersionOptional<basic::Bool, "10.1.0.0"_ver, Unbounded>
      hasVertexWeights{version};
  // arr1 = numVertices, arr2 = numWeightsPerVertex
  // Vertex weights were compulsory before 10.1.0.0.
  std::vector<std::vector<basic::Float>> vertexWeights{};

  std::vector<basic::UShort> stripLengths{};

  // Do we have strips/triangles data?
  VersionOptional<basic::Bool, "10.1.0.0"_ver, Unbounded> hasFaces{version};
  // arr1 = numStrips, arr2 = stripLengths for the corresponding strip
  // Strips were compulsory before 10.1.0.0, provided numStrips != 0
  std::vector<std::vector<basic::UShort>> strips{};
  // Triangles were compulsory before 10.1.0.0, and are only used if
  // numStrips == 0
  std::vector<compound::Triangle> triangles{};

  basic::Bool hasBoneIndices{};
  // arr1 = numVertices, arr2 = numWeightsPerVertex
  std::vector<std::vector<basic::Byte>> boneIndices{};

  explicit SkinPartition(Version version) : Versionable(version) {}
};

struct BoneVertData {
  basic::UShort index;
  basic::Float weight;
};

struct NiTransform {
  Matrix33 rotation{};
  Vector3 translation{};
  basic::Float scale = 1.0f;
};

std::istream &operator>>(std::istream &is, SizedString &t);
std::istream &operator>>(std::istream &is, String &t);
std::istream &operator>>(std::istream &is, ByteArray &t);
std::istream &operator>>(std::istream &is, ByteMatrix &t);
template<class T>
std::istream &operator>>(std::istream &is, Color3T<T> &t) {
  io::readBytes(is, t.r);
  io::readBytes(is, t.g);
  io::readBytes(is, t.b);
  return is;
}
template<class T>
std::istream &operator>>(std::istream &is, Color4T<T> &t) {
  io::readBytes(is, t.r);
  io::readBytes(is, t.g);
  io::readBytes(is, t.b);
  io::readBytes(is, t.a);
  return is;
}
std::istream &operator>>(std::istream &is, FilePath &t);
template<class NiObject>
std::istream &operator>>(std::istream &is, Footer<NiObject> &t);
std::istream &operator>>(std::istream &is, LODRange &t);
std::istream &operator>>(std::istream &is, MatchGroup &t);
template<class T>
std::istream &operator>>(std::istream &is, Vector3T<T> &t) {
  io::readBytes(is, t.x);
  io::readBytes(is, t.y);
  io::readBytes(is, t.z);
  return is;
}
template<class T>
std::istream &operator>>(std::istream &is, Vector4T<T> &t) {
  io::readBytes(is, t.x);
  io::readBytes(is, t.y);
  io::readBytes(is, t.z);
  io::readBytes(is, t.w);
  return is;
}
std::istream &operator>>(std::istream &is, Quaternion &t);
std::istream &operator>>(std::istream &is, hkQuaternion &t);
std::istream &operator>>(std::istream &is, Matrix22 &t);
std::istream &operator>>(std::istream &is, Matrix33 &t);
std::istream &operator>>(std::istream &is, Matrix34 &t);
std::istream &operator>>(std::istream &is, Matrix44 &t);
std::istream &operator>>(std::istream &is, hkMatrix3 &t);
std::istream &operator>>(std::istream &is, MipMap &t);
template<class NiNode>
std::istream &operator>>(std::istream &is, NodeSet<NiNode> &t) {
  io::readBytes(is, t.numNodes);
  t.nodes.reserve(t.numNodes);
  for (basic::UInt i = 0; i < t.numNodes; ++i) {
    basic::Ptr<NiNode> node{};
    io::readBytes(node.val);
    t.nodes.push_back(node);
  }
  return is;
}
std::istream &operator>>(std::istream &is, ShortString &t);
std::istream &operator>>(std::istream &is, ExportInfo &t);
std::istream &operator>>(std::istream &is, NiPlane &t);
std::istream &operator>>(std::istream &is, NiBound &t);
std::istream &operator>>(std::istream &is, BoxBV &t);
std::istream &operator>>(std::istream &is, CapsuleBV &t);
std::istream &operator>>(std::istream &is, UnionBV &t);
std::istream &operator>>(std::istream &is, HalfSpaceBV &t);
std::istream &operator>>(std::istream &is, EmptyBV &t);
std::istream &operator>>(std::istream &is, BoundingVolume &t);
std::istream &operator>>(std::istream &is, Header &t);
std::istream &operator>>(std::istream &is, MaterialData &t);
std::istream &operator>>(std::istream &is, Triangle &t);
std::istream &operator>>(std::istream &is, SkinPartition &t);
std::istream &operator>>(std::istream &is, BoneVertData &t);
std::istream &operator>>(std::istream &is, NiTransform &t);

} // namespace compound
} // namespace nif

#endif // OPENOBLIVION_NIF_COMPOUND_HPP
