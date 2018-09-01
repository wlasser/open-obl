#ifndef OPENOBLIVION_NIF_COMPOUND_HPP
#define OPENOBLIVION_NIF_COMPOUND_HPP

#include "io/read_bytes.hpp"
#include "nif/basic.hpp"
#include "nif/enum.hpp"
#include "nif/versionable.hpp"
#include <array>
#include <string>
#include <variant>
#include <vector>

namespace nif {

struct NiSourceTexture;

namespace compound {

using namespace nif::literals;

// BString
struct SizedString {
  basic::UInt length{};
  std::vector<basic::Char> value{};
  std::string str() const;
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
  SizedString string{};
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
static_assert(sizeof(Triangle) == 6, "Triangle must have no padding");

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

struct HavokFilter {
  // userVer2 < 16
  Enum::OblivionLayer layer{Enum::OblivionLayer::OL_STATIC};
  enum class Flags : uint8_t {
    NONE = 0,
    SCALED = 1u << 5u,
    NO_COLLISION = 1u << 6u,
    LINK = 1u << 7u
  };
  enum class Part : uint8_t {
    OTHER = 0,
    HEAD = 1,
    BODY = 2,
    SPINE1 = 3,
    SPINE2 = 4,
    LUPPERARM = 5,
    LFOREARM = 6,
    LHAND = 7,
    LTHIGH = 8,
    LCALF = 9,
    LFOOT = 10,
    RUPPERARM = 11,
    RFOREARM = 12,
    RHAND = 13,
    RTHIGH = 14,
    RCALF = 15,
    RFOOT = 16,
    TAIL = 17,
    SHIELD = 18,
    QUIVER = 19,
    WEAPON = 20,
    PONYTAIL = 21,
    WING = 22,
    PACK = 23,
    CHAIN = 24,
    ADDONHEAD = 25,
    ADDONCHEST = 26,
    ADDONARM = 27,
    ADDONLEG = 28
  };
  // 3 bits only
  Flags flags = Flags::NONE;
  // 5 bits only
  Part part = Part::OTHER;
  basic::UShort group{};
};

struct HavokMaterial : Versionable {
  VersionOptional<basic::UInt, Unbounded, "10.0.1.2"_ver> unknown{version};
  Enum::OblivionHavokMaterial material{};

  explicit HavokMaterial(Version version) : Versionable(version) {}
};

struct hkWorldObjCinfoProperty {
  basic::UInt data{};
  basic::UInt size{};
  basic::UInt capacityAndFlags{0x80000000};
};

// OpenGL texture coordinates. Origin is in the bottom left corner.
struct TexCoord {
  basic::Float u = 0.0f;
  basic::Float v = 0.0f;
};

struct TexDesc : Versionable {
  basic::Ref<NiSourceTexture> source{};
  Enum::TexClampMode clampMode{Enum::TexClampMode::WRAP_S_WRAP_T};
  Enum::TexFilterMode filterMode{Enum::TexFilterMode::FILTER_TRILERP};
  // Texture coordinate set in parent NiGeometryData that this slot uses
  basic::UInt uvSet{0};

  // Mipmap modifiers?
  VersionOptional<basic::Short, Unbounded, "10.4.0.1"_ver> ps2L{version};
  VersionOptional<basic::Short, Unbounded, "10.4.0.1"_ver> ps2K{version};

  VersionOptional<basic::UShort, Unbounded, "4.0.1.12"_ver> unknown{version};

  VersionOptional<basic::Bool, "10.1.0.0"_ver, Unbounded>
      hasTextureTransform{version, false};
  struct NiTextureTransform {
    TexCoord translation{};
    TexCoord scale{1.0f, 1.0f};
    basic::Float rotation{0.0f};
    Enum::TransformMethod transformMethod{};
    TexCoord center{};
  };
  std::optional<NiTextureTransform> textureTransform{};

  explicit TexDesc(Version version) : Versionable(version) {}
};

struct ShaderTexDesc : Versionable {
  basic::Bool hasMap{};
  struct Map : Versionable {
    TexDesc map{version};
    basic::UInt mapID{};
    explicit Map(Version version) : Versionable(version) {}
  };
  std::optional<Map> map{version};
  explicit ShaderTexDesc(Version version) : Versionable(version) {}
};

struct AdditionalDataInfo {
  basic::Int dataType{};
  basic::Int numChannelBytesPerElement{};
  // numVertices * numChannelBytesPerElement
  basic::Int numChannelBytes{};
  // Sum of numChannelBytesPerElement over all blockInfos
  basic::Int numTotalBytesPerElement{};
  // Block in which the channel is stored
  basic::Int blockIndex{};
  // Sum of all numChannelBytesPerElement over all preceding blockInfos
  basic::Int channelOffset{};
  basic::Byte unknown{};
};

struct AdditionalDataBlock {
  basic::Bool hasData{};
  basic::Int blockSize{};
  basic::Int numBlocks{};
  std::vector<basic::Int> blockOffsets{};
  basic::Int numData{};
  std::vector<basic::Int> dataSizes{};
  // arr1 = numData, arr2 = blockSize
  std::vector<std::vector<basic::Byte>> data{};
};

struct FormatPrefs {
  Enum::PixelLayout pixelLayout{};
  Enum::MipMapFormat mipMapFormat{Enum::MipMapFormat::MIP_FMT_DEFAULT};
  Enum::AlphaFormat alphaFormat{Enum::AlphaFormat::ALPHA_DEFAULT};
};

std::istream &operator>>(std::istream &is, SizedString &t);
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
    t.nodes.emplace_back();
    is >> t.nodes.back();
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
std::istream &operator>>(std::istream &is, HavokFilter &t);
std::istream &operator>>(std::istream &is, HavokMaterial &t);
std::istream &operator>>(std::istream &is, hkWorldObjCinfoProperty &t);
std::istream &operator>>(std::istream &is, TexCoord &t);
std::istream &operator>>(std::istream &is, TexDesc &t);
std::istream &operator>>(std::istream &is, ShaderTexDesc &t);
std::istream &operator>>(std::istream &is, AdditionalDataInfo &t);
std::istream &operator>>(std::istream &is, AdditionalDataBlock &t);
std::istream &operator>>(std::istream &is, FormatPrefs &t);

} // namespace compound
} // namespace nif

#endif // OPENOBLIVION_NIF_COMPOUND_HPP
