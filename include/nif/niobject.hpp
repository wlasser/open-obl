#ifndef OPENOBLIVION_NIF_NIOBJECT_HPP
#define OPENOBLIVION_NIF_NIOBJECT_HPP

#include "nif/basic.hpp"
#include "nif/compound.hpp"
#include "nif/versionable.hpp"
#include <limits>
#include <tuple>

namespace nif {

using namespace nif::literals;

struct NiObject {
  virtual ~NiObject() = 0;
};
inline NiObject::~NiObject() = default;

struct NiExtraData : NiObject, Versionable {

  VersionOptional<std::string, "10.0.1.0"_ver, Unbounded> name{version};

  VersionOptional<basic::Ref<NiExtraData>, Unbounded, "4.2.2.0"_ver>
      next{version};

  explicit NiExtraData(Version version) : Versionable(version) {}
};

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

  ~NiTimeController() override = 0;
};
inline NiTimeController::~NiTimeController() = default;

struct NiObjectNet : NiObject, Versionable {

  std::string name{};

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

  explicit NiObjectNet(Version version) : Versionable(version) {}
  ~NiObjectNet() override = 0;
};
inline NiObjectNet::~NiObjectNet() = default;

struct NiProperty : NiObjectNet {
  ~NiProperty() override = 0;
};
inline NiProperty::~NiProperty() = default;

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

  explicit NiAVObject(Version version) : NiObjectNet(version) {}
  ~NiAVObject() override = 0;
};
inline NiAVObject::~NiAVObject() = default;

struct NiCollisionObject : NiObject {
  basic::Ptr<NiAVObject> target{};
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

  explicit NiNode(Version version) : NiAVObject(version) {}
};

struct NiGeometryData : NiObject, Versionable {
  VersionOptional<basic::Int, "10.1.0.114"_ver, Unbounded> groupID{version};

  explicit NiGeometryData(Version version) : Versionable(version) {}
};

struct NiSkinPartition : NiObject {
  basic::UInt numSkinPartitionBlocks{};
  std::vector<compound::SkinPartition> skinPartitionBlocks{};
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
};

struct NiSkinInstance : NiObject, Versionable {
  basic::Ref<NiSkinData> data{};

  VersionOptional<basic::Ref<NiSkinPartition>, "10.1.0.101"_ver, Unbounded>
      skinPartition{version};

  basic::Ptr<NiNode> skeletonRoot{};
  basic::UInt numBones{};
  std::vector<basic::Ptr<NiNode>> bones{};

  explicit NiSkinInstance(Version version) : Versionable(version) {}
};

struct NiGeometry : NiAVObject {
  basic::Ref<NiGeometryData> data{};

  VersionOptional<basic::Ref<NiSkinInstance>, "3.3.0.13"_ver, Unbounded>
      skinInstance{version};

  VersionOptional<compound::MaterialData, "10.0.1.0"_ver, Unbounded>
      materialData{version};

  explicit NiGeometry(Version version) : NiAVObject(version) {}
};

struct NiTriBasedGeom : NiGeometry {
  ~NiTriBasedGeom() override = 0;
};
inline NiTriBasedGeom::~NiTriBasedGeom() = default;

struct NiTriShape : NiTriBasedGeom {};

} // namespace nif

#endif // OPENOBLIVION_NIF_NIOBJECT_HPP
