#ifndef OPENOBLIVION_NIF_NIOBJECT_HPP
#define OPENOBLIVION_NIF_NIOBJECT_HPP

#include "nif/basic.hpp"
#include "nif/compound.hpp"
#include "nif/versionable.hpp"
#include <istream>
#include <limits>
#include <tuple>
#include <vector>

namespace nif {

using namespace nif::literals;

struct NiObject {
  virtual ~NiObject() = 0;
  virtual void read(std::istream &) = 0;
};
inline NiObject::~NiObject() = default;
inline void NiObject::read(std::istream &) {}

struct NiExtraData : NiObject, Versionable {
  VersionOptional<std::string, "10.0.1.0"_ver, Unbounded> name{version};

  VersionOptional<basic::Ref<NiExtraData>, Unbounded, "4.2.2.0"_ver>
      next{version};

  void read(std::istream &is) override;
  explicit NiExtraData(Version version) : Versionable(version) {}
};

// Used to store tangents and bitangents
struct NiBinaryExtraData : NiExtraData {
  compound::ByteArray data{};

  void read(std::istream &is) override;
  explicit NiBinaryExtraData(Version version) : NiExtraData(version) {}
};

struct NiIntegerExtraData : NiExtraData {
  basic::UInt data{};

  void read(std::istream &is) override;
  explicit NiIntegerExtraData(Version version) : NiExtraData(version) {}
};

struct NiStringExtraData : NiExtraData {
  // Length of following string + 4
  VersionOptional<basic::UInt, Unbounded, "4.2.2.0"_ver>
      bytesRemaining{version};

  compound::String data{version};

  void read(std::istream &is) override;
  explicit NiStringExtraData(Version version) : NiExtraData(version) {}
};

struct BSXFlags : NiIntegerExtraData {
  enum class Flags : uint32_t {
    bAnimated = 0,
    bHavok = 1,
    bRagdoll = 1 << 1,
    bComplex = 1 << 2,
    bAddon = 1 << 3,
    bEditorMarker = 1 << 4,
    bDynamic = 1 << 5,
    bArticulated = 1 << 6,
    bIKTarget = 1 << 7,
    bExternalEmit = 1 << 8,
    bMagicShaderParticles = 1 << 9,
    bLights = 1 << 10,
    bBreakable = 1 << 11,
    bSearchedBreakable = 1 << 12
  };
  // Data is stored in NiIntegerExtraData::data
  void read(std::istream &is) override;
  explicit BSXFlags(Version version) : NiIntegerExtraData(version) {}
};
inline constexpr BSXFlags::Flags operator|(BSXFlags::Flags a,
                                           BSXFlags::Flags b) {
  return BSXFlags::Flags(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

struct NiObjectNet;

struct NiTimeController : NiObject {

  basic::Ref<NiTimeController> next{};

  // If Reverse and Clamp are unset, then Loop
  enum class Flag : basic::Flags {
    AppTime = 0,
    AppInit = 1,
    Reverse = 1 << 1,
    Clamp = 1 << 2,
    Active = 1 << 3,
    PlayBackwards = 1 << 4,
    IsManagerController = 1 << 5,
    ComputeScaledTime = 1 << 6,
    ForceUpdate = 1 << 7
  };
  Flag flags{};

  basic::Float frequency = 1.0f;
  basic::Float phase = 0.0f;
  basic::Float startTime = std::numeric_limits<float>::max();
  basic::Float stopTime = std::numeric_limits<float>::min();
  basic::Ptr<NiObjectNet> controllerTarget{};

  void read(std::istream &is) override;
  ~NiTimeController() override = 0;
};
inline NiTimeController::~NiTimeController() = default;

struct NiObjectNet : NiObject, Versionable {

  compound::SizedString name{};

  // Here, extraData entry has a link to another extraData entry,
  // i.e. extraData is as linked list.
  VersionOptional<basic::Ref<NiExtraData>, "3.0.0.0"_ver, "4.2.2.0"_ver>
      extraData{version};

  // Here, extraData is an array instead of a linked list
  VersionOptional<basic::UInt, "10.0.1.0"_ver, Unbounded>
      extraDataArrayLength{version};

  VersionOptional<std::vector<basic::Ref<NiExtraData>>, "10.0.1.0"_ver, Unbounded>
      extraDataArray{version};

  VersionOptional<basic::Ref<NiTimeController>, "3.0.0.0"_ver, Unbounded>
      controller{version};

  void read(std::istream &is) override;
  explicit NiObjectNet(Version version) : Versionable(version) {}
  ~NiObjectNet() override = 0;
};
inline NiObjectNet::~NiObjectNet() = default;

struct NiProperty : NiObjectNet {
  void read(std::istream &is) override;
  explicit NiProperty(Version version) : NiObjectNet(version) {}
  ~NiProperty() override = 0;
};
inline NiProperty::~NiProperty() = default;

struct NiMaterialProperty : NiProperty {
  VersionOptional<basic::Flags, "3.0.0.0"_ver, "10.0.1.2"_ver> flags{version};

  // userVer < 26
  compound::Color3 ambientColor{1.0f, 1.0f, 1.0f};
  // userVer < 26
  compound::Color3 diffuseColor{1.0f, 1.0f, 1.0f};
  compound::Color3 specularColor{1.0f, 1.0f, 1.0f};
  compound::Color3 emissiveColor{0.0f, 0.0f, 0.0f};
  basic::Float glossiness = 10.0f;
  // If not 1.0f, use NiAlphaProperty in the parent NiTriShape
  basic::Float alpha = 1.0f;
  // userVer > 21
  basic::Float emissiveMultiplier = 1.0f;

  void read(std::istream &is) override;
  explicit NiMaterialProperty(Version version) : NiProperty(version) {}
};

struct NiTexturingProperty : NiProperty {
  VersionOptional<basic::Flags, Unbounded, "10.0.1.2"_ver> flags{version};

  VersionOptional<Enum::ApplyMode, "3.3.0.13"_ver, "20.1.0.1"_ver>
      applyMode{version};

  basic::UInt textureCount{};

  basic::Bool hasBaseTexture{};
  compound::TexDesc baseTexture{version};

  basic::Bool hasDarkTexture{};
  compound::TexDesc darkTexture{version};

  basic::Bool hasDetailTexture{};
  compound::TexDesc detailTexture{version};

  basic::Bool hasGlossTexture{};
  compound::TexDesc glossTexture{version};

  basic::Bool hasGlowTexture{};
  compound::TexDesc glowTexture{version};

  // textureCount > 5
  VersionOptional<basic::Bool, "3.3.0.13"_ver, Unbounded>
      hasBumpTexture{version};
  struct BumpInfo : Versionable {
    compound::TexDesc bumpTexture{version};
    basic::Float lumaScale{};
    basic::Float lumaOffset{};
    compound::Matrix22 matrix{};
    explicit BumpInfo(Version version) : Versionable(version) {}
  };
  std::optional<BumpInfo> bumpTextureData{version};

  // textureCount > 6
  basic::Bool hasDecal0Texture{};
  compound::TexDesc decal0Texture{version};

  // textureCount > 7
  basic::Bool hasDecal1Texture{};
  compound::TexDesc decal1Texture{version};

  // textureCount > 8
  basic::Bool hasDecal2Texture{};
  compound::TexDesc decal2Texture{version};

  // textureCount > 9
  basic::Bool hasDecal3Texture{};
  compound::TexDesc decal3Texture{version};

  VersionOptional<basic::UInt, "10.0.1.0"_ver, Unbounded>
      numShaderTextures{version};
  VersionOptional<std::vector<compound::ShaderTexDesc>, "10.0.1.0"_ver, Unbounded>
      shaderTextures{version};

  void read(std::istream &is) override;
  explicit NiTexturingProperty(Version version) : NiProperty(version) {}
};

struct NiCollisionObject;

struct NiAVObject : NiObjectNet {

  basic::Flags flags = 0x0c;
  compound::Vector3 translation{};
  compound::Matrix33 rotation{};
  basic::Float scale = 1.0f;

  VersionOptional<compound::Vector3, Unbounded, "4.2.2.0"_ver>
      velocity{version};

  // userVer2 <= 34
  basic::UInt numProperties = 0;
  std::vector<basic::Ref<NiProperty>> properties{};

  VersionOptional<basic::Bool, "3.0.0.0"_ver, "4.2.2.0"_ver>
      hasBoundingVolume{version};

  VersionOptional<compound::BoundingVolume, "3.0.0.0"_ver, "4.2.2.0"_ver>
      boundingVolume{version};

  VersionOptional<basic::Ref<NiCollisionObject>, "10.0.1.0"_ver, Unbounded>
      collisionObject{version};

  void read(std::istream &is) override;
  explicit NiAVObject(Version version) : NiObjectNet(version) {}
  ~NiAVObject() override = 0;
};
inline NiAVObject::~NiAVObject() = default;

struct NiCollisionObject : NiObject {
  basic::Ptr<NiAVObject> target{};
  void read(std::istream &is) override;
};

struct NiNode;

struct NiDynamicEffect : NiAVObject {
  // userVer2 < 130
  // If true, effect is applied to affected nodes during rendering
  VersionOptional<basic::Bool, "10.1.0.106"_ver, Unbounded>
      switchState{version, true};

  // The versioning is weird here. For our purposes, we can assume that
  // numAffectedNodes and affectedNodes both exist iff
  // ver < 4.0.0.2 || ver > 10.1.0.0. We can't support such
  // a requirement directly without having two copies of the same variables with
  // different names, but we can support the complement of the requirement,
  // implemented using the empty member noAffectedNodes.
  VersionOptional<std::tuple<>, "3.3.0.14"_ver, "10.0.255.255"_ver>
      noAffectedNodes{version};
  basic::UInt numAffectedNodes{version};
  std::vector<basic::Ptr<NiNode>> affectedNodes{version};

  void read(std::istream &is) override;
  explicit NiDynamicEffect(Version version) : NiAVObject(version) {}
  ~NiDynamicEffect() override = 0;
};
inline NiDynamicEffect::~NiDynamicEffect() = default;

struct NiNode : NiAVObject {
  basic::UInt numChildren{};
  std::vector<basic::Ref<NiAVObject>> children{};

  // userVer2 < 130
  basic::UInt numEffects{};
  std::vector<basic::Ref<NiDynamicEffect>> effects{};

  void read(std::istream &is) override;
  explicit NiNode(Version version) : NiAVObject(version) {}
};

struct AbstractAdditionalGeometryData : NiObject {
  void read(std::istream &is) override;
  ~AbstractAdditionalGeometryData() override = 0;
};
inline AbstractAdditionalGeometryData::~AbstractAdditionalGeometryData() = default;

struct NiAdditionalGeometryData : AbstractAdditionalGeometryData {
  basic::UShort numVertices{};

  basic::UInt numBlockInfos{};
  std::vector<compound::AdditionalDataInfo> blockInfos{};

  basic::Int numBlocks{};
  std::vector<compound::AdditionalDataBlock> blocks{};

  void read(std::istream &is) override;
};

struct NiGeometryData : NiObject, Versionable {
  VersionOptional<basic::Int, "10.1.0.114"_ver, Unbounded> groupID{version};

  basic::UShort numVertices{};

  // Used with NiCollision objects with OBB or TRI is set
  VersionOptional<basic::Byte, "10.1.0.0"_ver, Unbounded> keepFlags{version};

  VersionOptional<basic::Byte, "10.1.0.0"_ver, Unbounded>
      compressFlags{version};

  basic::Bool hasVertices = true;
  std::vector<compound::Vector3> vertices{};

  VersionOptional<Enum::VectorFlags, "10.0.1.0"_ver, Unbounded>
      vectorFlags{version};

  basic::Bool hasNormals{};
  std::vector<compound::Vector3> normals{};

  // if (hasNormals && (vectorFlags & VF_Has_Tangents))
  std::vector<compound::Vector3> tangents{};
  std::vector<compound::Vector3> bitangents{};

  // Bounding box center and maximum distance from center to any vertex
  compound::Vector3 center{};
  basic::Float radius{};

  basic::Bool hasVertexColors{};
  std::vector<compound::Color4> vertexColors{};

  // Top 10 bits are flags. Bit 12 is set if tangents or bitangents are present.
  VersionOptional<basic::UShort, Unbounded, "4.2.2.0"_ver> numUVSets{version};

  VersionOptional<basic::Bool, Unbounded, "4.0.0.2"_ver> hasUV{version};

  // Texture coordinates with OpenGL convention
  // arr1 = (numUVSets & 63) | (vectorFlags & 63), arr2 = numVertices
  std::vector<std::vector<compound::TexCoord>> uvSets{};

  VersionOptional<Enum::ConsistencyType, "10.0.1.0"_ver, Unbounded>
      consistencyFlags{version, Enum::ConsistencyType::CT_MUTABLE};

  VersionOptional<basic::Ref<AbstractAdditionalGeometryData>, "20.0.0.0.4"_ver, Unbounded>
      additionalData{version};

  void read(std::istream &is) override;
  explicit NiGeometryData(Version version) : Versionable(version) {}
};

struct NiTriBasedGeomData : NiGeometryData {
  basic::UShort numTriangles{};

  void read(std::istream &is) override;
  explicit NiTriBasedGeomData(Version version) : NiGeometryData(version) {}
  ~NiTriBasedGeomData() override = 0;
};
inline NiTriBasedGeomData::~NiTriBasedGeomData() = default;

struct NiTriShapeData : NiTriBasedGeomData {
  basic::UInt numTrianglePoints{};

  VersionOptional<basic::Bool, "10.1.0.0"_ver, Unbounded> hasTriangles{version};
  std::vector<compound::Triangle> triangles{};

  // Number of shared normal groups
  basic::UShort numMatchGroups{};
  std::vector<compound::MatchGroup> matchGroups{};

  void read(std::istream &is) override;
  explicit NiTriShapeData(Version version) : NiTriBasedGeomData(version) {}
};

struct NiSkinPartition : NiObject, Versionable {
  basic::UInt numSkinPartitionBlocks{};
  std::vector<compound::SkinPartition> skinPartitionBlocks{};
  void read(std::istream &is) override;
  explicit NiSkinPartition(Version version) : Versionable(version) {}
};

struct NiSkinData : NiObject, Versionable {
  compound::NiTransform skinTransform{};

  basic::UInt numBones{};

  // Optionally link an NiSkinPartition for hardware-acceleration info
  VersionOptional<basic::Ref<NiSkinPartition>, "4.0.0.2"_ver, "10.1.0.0"_ver>
      skinPartition{version};

  VersionOptional<basic::Bool, "4.2.1.0"_ver, Unbounded>
      hasVertexWeights{version, true};

  struct BoneData {
    // Offset of the skin from this bone in the bind position
    compound::NiTransform skinTransform{};
    compound::Vector3 boundingSphereOffset{};
    basic::Float boundingSphereRadius{};
    basic::UShort numVertices{};
    // Present if hasVertexWeights is true or absent
    std::vector<compound::BoneVertData> vertexWeights{};
  };
  // Contains offset data for each node the skin is influenced by
  // arg = hasVertexWeights
  std::vector<BoneData> boneList{};

  void read(std::istream &is) override;
  explicit NiSkinData(Version version) : Versionable(version) {}
};

struct NiSkinInstance : NiObject, Versionable {
  basic::Ref<NiSkinData> data{};

  VersionOptional<basic::Ref<NiSkinPartition>, "10.1.0.101"_ver, Unbounded>
      skinPartition{version};

  basic::Ptr<NiNode> skeletonRoot{};
  basic::UInt numBones{};
  std::vector<basic::Ptr<NiNode>> bones{};

  void read(std::istream &is) override;
  explicit NiSkinInstance(Version version) : Versionable(version) {}
};

struct NiGeometry : NiAVObject {
  basic::Ref<NiGeometryData> data{};

  VersionOptional<basic::Ref<NiSkinInstance>, "3.3.0.13"_ver, Unbounded>
      skinInstance{version};

  VersionOptional<compound::MaterialData, "10.0.1.0"_ver, Unbounded>
      materialData{version};

  void read(std::istream &is) override;
  explicit NiGeometry(Version version) : NiAVObject(version) {}
};

struct NiTriBasedGeom : NiGeometry {
  void read(std::istream &is) override;
  explicit NiTriBasedGeom(Version version) : NiGeometry(version) {}
  ~NiTriBasedGeom() override = 0;
};
inline NiTriBasedGeom::~NiTriBasedGeom() = default;

struct NiTriShape : NiTriBasedGeom {
  explicit NiTriShape(Version version) : NiTriBasedGeom(version) {}
  void read(std::istream &is) override;
};

struct NiTexture : NiObjectNet {
  void read(std::istream &is) override;
  explicit NiTexture(Version version) : NiObjectNet(version) {}
  ~NiTexture() override = 0;
};
inline NiTexture::~NiTexture() = default;

struct NiSourceTexture : NiTexture {
  struct ExternalTextureFile : Versionable {
    compound::FilePath filename{version};

    VersionOptional<basic::Ref<NiObject>, "10.1.0.0"_ver, Unbounded>
        unknownRef{version};

    explicit ExternalTextureFile(Version version) : Versionable(version) {}
  };

  struct InternalTextureFile : Versionable {
    VersionOptional<basic::Byte, Unbounded, "10.0.1.0"_ver>
        unknownByte{version};

    VersionOptional<compound::FilePath, "10.1.0.0"_ver, Unbounded>
        filename{version, compound::FilePath{version}};

    // TODO: Support NiPixelFormat. I think it might never be used?
    // basic::Ref<NiPixelFormat> pixelData{};

    explicit InternalTextureFile(Version version) : Versionable(version) {}
  };

  basic::Bool useExternal{true};

  std::variant<ExternalTextureFile, InternalTextureFile>
      textureFileData{ExternalTextureFile{version}};

  compound::FormatPrefs formatPrefs{};

  basic::Bool isStatic{true};

  VersionOptional<basic::Bool, "10.1.0.103"_ver, Unbounded>
      directRender{version};

  void read(std::istream &is) override;
  explicit NiSourceTexture(Version version) : NiTexture(version) {}
};
} // namespace nif

#endif // OPENOBLIVION_NIF_NIOBJECT_HPP
