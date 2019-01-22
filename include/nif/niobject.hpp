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

struct NiFloatData : NiObject {
  compound::KeyGroup<basic::Float> keys{};

  void read(std::istream &is) override;
};

struct NiKeyframeData : NiObject, Versionable {
  template<Enum::KeyType Arg>
  using KeysVector = std::vector<compound::QuatKey<Arg>>;

  // If rotationType == 4 then this must be 1, and the actual number of keys is
  // stored in xyzRotations
  basic::UInt numRotationKeys{};

  // if (numRotationKeys != 0)
  // Value of 4 => separate xyz, not quaternions
  Enum::KeyType rotationType{};

  // if (rotationType != 4)
  // arg = rotationType
  std::variant<KeysVector<Enum::KeyType::NONE>,
               KeysVector<Enum::KeyType::LINEAR_KEY>,
               KeysVector<Enum::KeyType::QUADRATIC_KEY>,
               KeysVector<Enum::KeyType::TBC_KEY>,
               KeysVector<Enum::KeyType::XYZ_ROTATION_KEY>,
               KeysVector<Enum::KeyType::CONST_KEY>> quaternionKeys{};

  template<std::size_t I>
  void readKeys(std::istream &is) {
    auto v = quaternionKeys.template emplace<I>(numRotationKeys);
    for (auto &key : v) is >> key;
  }

  // if (rotationType == 4)
  VersionOptional<basic::Float, Unbounded, "10.1.0.0"_ver> order{version};

  // if (rotationType == 4)
  std::array<compound::KeyGroup<basic::Float>, 3> xyzRotations{};

  compound::KeyGroup<compound::Vector3> translations{};

  compound::KeyGroup<basic::Float> scales{};

  void read(std::istream &is) override;
  explicit NiKeyframeData(Version version) : Versionable(version) {}
};

struct NiTransformData : NiKeyframeData {
  void read(std::istream &is) override;
  explicit NiTransformData(Version version) : NiKeyframeData(version) {}
};

struct NiPosData : NiObject {
  compound::KeyGroup<compound::Vector3> data{};

  void read(std::istream &is) override;
};

struct NiStringPalette : NiObject {
  compound::StringPalette palette{};

  void read(std::istream &is) override;
};

struct NiExtraData : NiObject, Versionable {
  VersionOptional<compound::SizedString, "10.0.1.0"_ver, Unbounded>
      name{version};

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

  compound::SizedString data{};

  void read(std::istream &is) override;
  explicit NiStringExtraData(Version version) : NiExtraData(version) {}
};

struct NiTextKeyExtraData : NiExtraData {
  VersionOptional<basic::UInt, Unbounded, "4.2.2.0"_ver> unknownInt1{version};

  basic::UInt numTextKeys{};

  std::vector<compound::Key<compound::SizedString, Enum::KeyType::LINEAR_KEY>>
      textKeys{};

  void read(std::istream &is) override;
  explicit NiTextKeyExtraData(Version version) : NiExtraData(version) {}
};

struct BSBound : NiExtraData {
  compound::Vector3 center{};
  compound::Vector3 dimensions{};

  void read(std::istream &is) override;
  explicit BSBound(Version version) : NiExtraData(version) {}
};

struct NiInterpolator : NiObject {
  void read(std::istream &is) override;
  ~NiInterpolator() override = 0;
};
inline NiInterpolator::~NiInterpolator() = default;

struct NiKeyBasedInterpolator : NiInterpolator {
  void read(std::istream &is) override;
  ~NiKeyBasedInterpolator() override = 0;
};
inline NiKeyBasedInterpolator::~NiKeyBasedInterpolator() = default;

struct NiFloatInterpolator : NiKeyBasedInterpolator {
  basic::Float value{std::numeric_limits<float>::lowest()};
  basic::Ref<NiFloatData> data{};

  void read(std::istream &is) override;
};

struct NiTransformInterpolator : NiKeyBasedInterpolator, Versionable {
  compound::NiQuatTransform transform{version};
  basic::Ref<NiTransformData> data{};

  void read(std::istream &is) override;
  explicit NiTransformInterpolator(Version version) : Versionable(version) {}
};

struct NiPoint3Interpolator : NiKeyBasedInterpolator {
  compound::Vector3 value{std::numeric_limits<float>::lowest(),
                          std::numeric_limits<float>::lowest(),
                          std::numeric_limits<float>::lowest()};
  basic::Ref<NiPosData> data{};

  void read(std::istream &is) override;
};

struct NiBlendInterpolator : NiInterpolator, Versionable {
  // This is always present, with the same type, but changes location based on
  // the version. This version should be written to internally, io should use
  // the versioned components.
  basic::Float weightThreshold{};

  VersionOptional<Enum::InterpBlendFlags, "10.1.0.112"_ver, Unbounded>
      flags{version};

  struct ArrayParamsL {
    basic::UShort arraySize{};
    basic::UShort arrayGrowBy{};
    friend std::istream &operator>>(std::istream &is, ArrayParamsL &t) {
      io::readBytes(is, t.arraySize);
      io::readBytes(is, t.arrayGrowBy);
      return is;
    }
  };
  struct ArrayParamsR {
    basic::Byte arraySize;
    friend std::istream &operator>>(std::istream &is, ArrayParamsR &t) {
      io::readBytes(is, t.arraySize);
      return is;
    }
  };
  VersionEither<ArrayParamsL, ArrayParamsR, "10.1.0.110"_ver, Unbounded>
      arrayParams{version};

 protected:
  VersionOptional<basic::Float *, "10.1.0.112"_ver, Unbounded>
      weightThresholdR{version, &weightThreshold};
 public:

  struct UnmanagedData {
    basic::Byte interpCount{};
    basic::Byte singleIndex{0xff};
    basic::Char highPriority{std::numeric_limits<basic::Char>::lowest()};
    basic::Char nextHighPriority{std::numeric_limits<basic::Char>::lowest()};
    basic::Float singleTime{std::numeric_limits<basic::Float>::lowest()};
    basic::Float highWeightsSum{std::numeric_limits<basic::Float>::lowest()};
    basic::Float
        nextHighWeightsSum{std::numeric_limits<basic::Float>::lowest()};
    basic::Float highEaseSpinner{std::numeric_limits<basic::Float>::lowest()};
    std::vector<compound::InterpBlendItem> interpArrayItems{};
  };
  // if (flags & 1) == 0
  VersionOptional<UnmanagedData, "10.1.0.112"_ver, Unbounded>
      unmanagedData{version};

  VersionOptional<std::vector<compound::InterpBlendItem>,
      Unbounded, "10.1.0.111"_ver>
      interpArrayItems{version};

  VersionOptional<basic::Bool, Unbounded, "10.1.0.111"_ver>
      managerControlled{version};

 protected:
  VersionOptional<basic::Float *, Unbounded, "10.1.0.111"_ver>
      weightThresholdL{version, &weightThreshold};
 public:

  VersionOptional<basic::Bool, Unbounded, "10.1.0.111"_ver>
      onlyUseHeighestWeight{version};

  VersionOptional<VersionEither<basic::UShort,
                                basic::Byte,
                                "10.1.0.110"_ver,
                                Unbounded>, Unbounded, "10.1.0.111"_ver>
      interpCount{version,
                  VersionEither<basic::UShort,
                                basic::Byte,
                                "10.1.0.110"_ver,
                                Unbounded>(version)};

  VersionOptional<VersionEither<basic::UShort,
                                basic::Byte,
                                "10.1.0.110"_ver,
                                Unbounded>, Unbounded, "10.1.0.111"_ver>
      singleIndex{version,
                  VersionEither<basic::UShort,
                                basic::Byte,
                                "10.1.0.110"_ver,
                                Unbounded>(version)};

  VersionOptional<basic::Ref<NiInterpolator>, "10.1.0.108"_ver,
                                              "10.1.0.111"_ver>
      singleInterpolator{version};

  VersionOptional<basic::Float, "10.1.0.108"_ver, "10.1.0.111"_ver>
      singleTime{version};

  VersionOptional<VersionEither<basic::Int,
                                basic::Byte,
                                "10.1.0.110"_ver,
                                Unbounded>, Unbounded, "10.1.0.111"_ver>
      highPriority{version,
                   VersionEither<basic::Int,
                                 basic::Byte,
                                 "10.1.0.110"_ver,
                                 Unbounded>(version)};

  VersionOptional<VersionEither<basic::Int,
                                basic::Byte,
                                "10.1.0.110"_ver,
                                Unbounded>, Unbounded, "10.1.0.111"_ver>
      nextHighPriority{version,
                       VersionEither<basic::Int,
                                     basic::Byte,
                                     "10.1.0.110"_ver,
                                     Unbounded>(version)};

  void read(std::istream &is) override;
  explicit NiBlendInterpolator(Version version) : Versionable(version) {}
  ~NiBlendInterpolator() override = 0;
};
inline NiBlendInterpolator::~NiBlendInterpolator() = default;

struct NiBlendPoint3Interpolator : NiBlendInterpolator {
  compound::Vector3 value{std::numeric_limits<float>::lowest(),
                          std::numeric_limits<float>::lowest(),
                          std::numeric_limits<float>::lowest()};

  void read(std::istream &is) override;
  explicit NiBlendPoint3Interpolator(Version version)
      : NiBlendInterpolator(version) {}
};

struct BSXFlags : NiIntegerExtraData {
  enum class Flags : uint32_t {
    bNone = 0u,
    bAnimated = 1u << 0u,
    bHavok = 1u << 1u,
    bRagdoll = 1u << 2u,
    bComplex = 1u << 3u,
    bAddon = 1u << 4u,
    bEditorMarker = 1u << 5u,
    bDynamic = 1u << 6u,
    bArticulated = 1u << 7u,
    bIKTarget = 1u << 8u,
    bExternalEmit = 1u << 9u,
    bMagicShaderParticles = 1u << 10u,
    bLights = 1u << 11u,
    bBreakable = 1u << 12u,
    bSearchedBreakable = 1u << 13u
  };
  // Data is stored in NiIntegerExtraData::data
  void read(std::istream &is) override;
  explicit BSXFlags(Version version) : NiIntegerExtraData(version) {}
};
inline constexpr BSXFlags::Flags operator|(BSXFlags::Flags a,
                                           BSXFlags::Flags b) {
  return BSXFlags::Flags(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
inline constexpr BSXFlags::Flags operator&(BSXFlags::Flags a,
                                           BSXFlags::Flags b) {
  return BSXFlags::Flags(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

struct NiSequence : NiObject, Versionable {
  compound::SizedString name{};

  VersionOptional<compound::SizedString, Unbounded, "10.1.0.103"_ver>
      accumRootName{version};

  VersionOptional<basic::Ref<NiTextKeyExtraData>, Unbounded, "10.1.0.103"_ver>
      textKeys{version};

  basic::UInt numControlledBlocks{};

  VersionOptional<basic::UInt, "10.1.0.106"_ver, Unbounded>
      arrayGrowBy{version};

  std::vector<compound::ControlledBlock> controlledBlocks{};

  void read(std::istream &is) override;
  explicit NiSequence(Version version) : Versionable(version) {}
};

struct NiControllerManager;

struct NiControllerSequence : NiSequence {
  VersionOptional<basic::Float, "10.1.0.106"_ver, Unbounded>
      weight{version, 1.0f};

  VersionOptional<basic::Ref<NiTextKeyExtraData>, "10.1.0.106"_ver, Unbounded>
      textKeys{version};

  VersionOptional<Enum::CycleType, "10.1.0.106"_ver, Unbounded>
      cycleType{version};

  VersionOptional<basic::Float, "10.1.0.106"_ver, Unbounded>
      frequency{version, 1.0f};

  VersionOptional<basic::Float, "10.1.0.106"_ver, "10.4.0.1"_ver>
      phase{version};

  VersionOptional<basic::Float, "10.1.0.106"_ver, Unbounded>
      startTime{version, std::numeric_limits<float>::max()};

  VersionOptional<basic::Float, "10.1.0.106"_ver, Unbounded>
      stopTime{version, std::numeric_limits<float>::lowest()};

  VersionOptional<basic::Bool, "10.1.0.106"_ver, "10.1.0.106"_ver>
      playBackwards{version};

  VersionOptional<basic::Ptr<NiControllerManager>, "10.1.0.106"_ver, Unbounded>
      manager{version};

  VersionOptional<compound::SizedString, "10.1.0.106"_ver, Unbounded>
      accumRootName{version};

  VersionOptional<basic::Ref<NiStringPalette>, "10.1.0.113"_ver, "20.1.0.0"_ver>
      stringPalette{version};

  void read(std::istream &is) override;
  explicit NiControllerSequence(Version version) : NiSequence(version) {}
};

struct NiAVObjectPalette : NiObject {
  void read(std::istream &is) override;
  ~NiAVObjectPalette() override = 0;
};
inline NiAVObjectPalette::~NiAVObjectPalette() = default;

struct NiAVObject;

struct NiDefaultAVObjectPalette : NiAVObjectPalette {
  basic::Ptr<NiAVObject> scene{};
  basic::UInt numObjects{};
  std::vector<compound::AVObject> objects{};

  void read(std::istream &is) override;
};

struct NiObjectNet;

struct NiTimeController : NiObject {
  basic::Ref<NiTimeController> next{};

  // If Reverse and Clamp are unset, then Loop
  enum class Flag : basic::Flags {
    AppTime = 0u,
    AppInit = 1u << 0u,
    Reverse = 1u << 1u,
    Clamp = 1u << 2u,
    Active = 1u << 3u,
    PlayBackwards = 1u << 4u,
    IsManagerController = 1u << 5u,
    ComputeScaledTime = 1u << 6u,
    ForceUpdate = 1u << 7u
  };
  Flag flags{};

  basic::Float frequency = 1.0f;
  basic::Float phase = 0.0f;
  basic::Float startTime = std::numeric_limits<float>::max();
  basic::Float stopTime = std::numeric_limits<float>::lowest();
  basic::Ptr<NiObjectNet> controllerTarget{};

  void read(std::istream &is) override;
  ~NiTimeController() override = 0;
};
inline NiTimeController::~NiTimeController() = default;

struct NiInterpController : NiTimeController, Versionable {
  VersionOptional<basic::Bool, "10.1.0.104"_ver, "10.1.0.108"_ver>
      managerControlled{version};

  void read(std::istream &is) override;
  explicit NiInterpController(Version version) : Versionable(version) {}
  ~NiInterpController() override = 0;
};
inline NiInterpController::~NiInterpController() = default;

struct NiMultiTargetTransformController : NiInterpController {
  basic::UShort numExtraTargets{};
  std::vector<basic::Ptr<NiAVObject>> extraTargets{};

  void read(std::istream &is) override;
  explicit NiMultiTargetTransformController(Version version)
      : NiInterpController(version) {}
};

struct NiSingleInterpController : NiInterpController {
  VersionOptional<basic::Ref<NiInterpolator>, "10.1.0.104"_ver, Unbounded>
      interpolator{version};

  void read(std::istream &is) override;
  explicit NiSingleInterpController(Version version)
      : NiInterpController(version) {}
  ~NiSingleInterpController() override = 0;
};
inline NiSingleInterpController::~NiSingleInterpController() = default;

struct NiPoint3InterpController : NiSingleInterpController {
  void read(std::istream &is) override;
  explicit NiPoint3InterpController(Version version) : NiSingleInterpController(
      version) {}
  ~NiPoint3InterpController() override = 0;
};
inline NiPoint3InterpController::~NiPoint3InterpController() = default;

struct NiMaterialColorController : NiPoint3InterpController {
  VersionOptional<Enum::MaterialColor, "10.1.0.0"_ver, Unbounded>
      targetColor{version};

  VersionOptional<basic::Ref<NiPosData>, Unbounded, "10.1.0.103"_ver>
      data{version};

  void read(std::istream &is) override;
  explicit NiMaterialColorController(Version version)
      : NiPoint3InterpController(version) {}
};

struct NiKeyframeController : NiSingleInterpController {
  VersionOptional<basic::Ref<NiKeyframeData>, Unbounded, "10.1.0.103"_ver>
      keyframeData{version};

  void read(std::istream &is) override;
  explicit NiKeyframeController(Version version)
      : NiSingleInterpController(version) {}
};

struct NiTransformController : NiKeyframeController {
  void read(std::istream &is) override;
  explicit NiTransformController(Version version)
      : NiKeyframeController(version) {}
};

struct NiControllerManager : NiTimeController {
  // If enabled the manager treats all sequence data as absolute, not relative
  basic::Bool cumulative{};
  basic::UInt numControllerSequences{};
  std::vector<basic::Ref<NiControllerSequence>> controllerSequences{};
  basic::Ref<NiDefaultAVObjectPalette> objectPalette{};

  void read(std::istream &is) override;
};

using NiExtraDataArray = std::vector<basic::Ref<NiExtraData>>;

struct NiObjectNet : NiObject, Versionable {
  compound::SizedString name{};

  // Here, extraData entry has a link to another extraData entry,
  // i.e. extraData is as linked list.
  VersionOptional<basic::Ref<NiExtraData>, "3.0.0.0"_ver, "4.2.2.0"_ver>
      extraData{version};

  // Here, extraData is an array instead of a linked list
  VersionOptional<basic::UInt, "10.0.1.0"_ver, Unbounded>
      extraDataArrayLength{version};

  VersionOptional<NiExtraDataArray, "10.0.1.0"_ver, Unbounded>
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

struct NiStencilProperty : NiProperty {
  VersionOptional<basic::Flags, Unbounded, "10.0.1.2"_ver> flags{version};
  basic::Bool stencilEnabled{};
  Enum::StencilCompareMode stencilFunction{};
  basic::UInt stencilRef{};
  basic::UInt stencilMask{0xffffffff};
  Enum::StencilAction failAction{};
  Enum::StencilAction zfailAction{};
  Enum::StencilAction passAction{};
  Enum::StencilDrawMode drawMode{Enum::StencilDrawMode::DRAW_BOTH};

  void read(std::istream &is) override;
  explicit NiStencilProperty(Version version) : NiProperty(version) {}
};

struct NiVertexColorProperty : NiProperty {
  basic::Flags flags{};
  Enum::VertMode vertexMode{Enum::VertMode::VERT_MODE_SRC_AMB_DIF};
  Enum::LightMode lightMode{Enum::LightMode::LIGHT_MODE_EMI_AMB_DIF};

  void read(std::istream &is) override;
  explicit NiVertexColorProperty(Version version) : NiProperty(version) {}
};

struct NiAlphaProperty : NiProperty {
  enum class BlendMode : uint8_t {
    GL_ONE = 0,
    GL_ZERO = 1u,
    GL_SRC_COLOR = 2u,
    GL_ONE_MINUS_SRC_COLOR = 3u,
    GL_DST_COLOR = 4u,
    GL_ONE_MINUS_DST_COLOR = 5u,
    GL_SRC_ALPHA = 6u,
    GL_ONE_MINUS_SRC_ALPHA = 7u,
    GL_DST_ALPHA = 8u,
    GL_ONE_MINUS_DST_ALPHA = 9u,
    GL_SRC_ALPHA_SATURATE = 10u
  };
  enum class TestMode : uint8_t {
    GL_ALWAYS = 0,
    GL_LESS = 1u,
    GL_EQUAL = 2u,
    GL_LEQUAL = 3u,
    GL_GREATER = 4u,
    GL_NOTEQUAL = 5u,
    GL_GEQUAL = 6u,
    GL_NEVER = 7u
  };
  // The flags are stored in a uint16_t but split up as below. Default is
  // 0b00'0'100'1'0111'0110'0
  // bit 0
  bool alphaBlendingEnabled{false};
  // bit 1-4
  BlendMode sourceBlendMode{BlendMode::GL_SRC_ALPHA};
  // bit 5-8
  BlendMode destinationBlendMode{BlendMode::GL_ONE_MINUS_SRC_ALPHA};
  // bit 9
  bool alphaTestEnabled{true};
  // bit 10-12
  TestMode alphaTestMode{TestMode::GL_GREATER};
  // bit 13
  bool disableTriangleSorting{false};

  uint8_t threshold{};

  void read(std::istream &is) override;
  explicit NiAlphaProperty(Version version) : NiProperty(version) {}
};

struct NiSpecularProperty : NiProperty {
  enum class Flags : basic::Flags {
    DisableSpecular = 0,
    EnableSpecular = 1u
  };
  Flags flags{Flags::EnableSpecular};

  void read(std::istream &is) override;
  explicit NiSpecularProperty(Version version) : NiProperty(version) {}
};

struct NiCollisionObject;

using NiPropertyArray = std::vector<basic::Ref<NiProperty>>;

struct NiAVObject : NiObjectNet {

  basic::Flags flags = 0x0c;
  compound::Vector3 translation{};
  compound::Matrix33 rotation{};
  basic::Float scale = 1.0f;

  VersionOptional<compound::Vector3, Unbounded, "4.2.2.0"_ver>
      velocity{version};

  // userVer2 <= 34
  basic::UInt numProperties = 0;
  NiPropertyArray properties{};

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

  // Lower 5 bits replace numUVSets
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

  VersionOptional<basic::UShort, Unbounded, "4.2.2.0"_ver> numUVSets{version};

  VersionOptional<basic::Bool, Unbounded, "4.0.0.2"_ver> hasUV{version};

  // Texture coordinates with OpenGL convention
  // arr1 = (numUVSets & 63) | (vectorFlags & 63), arr2 = numVertices
  std::vector<std::vector<compound::TexCoord>> uvSets{};

  VersionOptional<Enum::ConsistencyType, "10.0.1.0"_ver, Unbounded>
      consistencyFlags{version, Enum::ConsistencyType::CT_MUTABLE};

  VersionOptional<basic::Ref<AbstractAdditionalGeometryData>, "20.0.0.4"_ver, Unbounded>
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

struct NiTriStripsData : NiTriBasedGeomData {
  basic::UShort numStrips{};
  std::vector<basic::UShort> stripLengths{};

  VersionOptional<basic::Bool, "10.0.1.3"_ver, Unbounded> hasPoints{version};
  // arr1 = numStrips, arr2 = stripLengths
  std::vector<std::vector<basic::UShort>> points{};

  void read(std::istream &is) override;
  explicit NiTriStripsData(Version version) : NiTriBasedGeomData(version) {}
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

struct NiTriStrips : NiTriBasedGeom {
  void read(std::istream &is) override;
  explicit NiTriStrips(Version version) : NiTriBasedGeom(version) {}
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

struct NiBoneLODController : NiTimeController, Versionable {
  basic::UInt lod{};
  basic::UInt numLods{};
  basic::UInt numNodeGroups{};
  // arr1 = numNodeGroups
  std::vector<compound::NodeSet<NiNode>> nodeGroups{};

  void read(std::istream &is) override;
  explicit NiBoneLODController(Version version) : Versionable(version) {}
};

struct NiBSBoneLODController : NiBoneLODController {
  void read(std::istream &is) override;
  explicit NiBSBoneLODController(Version version)
      : NiBoneLODController(version) {}
};

namespace hk {

struct PackedNiTriStripsData;

} // namespace hk

namespace bhk {

struct RefObject : nif::NiObject {
  void read(std::istream &is) override;
  ~RefObject() override = 0;
};
inline RefObject::~RefObject() = default;

struct Serializable : bhk::RefObject {
  void read(std::istream &is) override;
  ~Serializable() override = 0;
};
inline Serializable::~Serializable() = default;

struct Shape : bhk::Serializable {
  void read(std::istream &is) override;
  ~Shape() override = 0;
};
inline Shape::~Shape() = default;

struct TransformShape : bhk::Shape, Versionable {
  basic::Ref<bhk::Shape> shape{};
  compound::HavokMaterial material{version};
  basic::Float radius{};
  std::array<basic::Byte, 8> unused{};
  compound::Matrix44 transform{};

  void read(std::istream &is) override;
  explicit TransformShape(Version version) : Versionable(version) {}
};

struct SphereRepShape : bhk::Shape, Versionable {
  compound::HavokMaterial material{version};
  basic::Float radius{};

  void read(std::istream &is) override;
  explicit SphereRepShape(Version version) : Versionable(version) {}
  ~SphereRepShape() override = 0;
};
inline SphereRepShape::~SphereRepShape() = default;

struct ConvexShape : bhk::SphereRepShape {
  void read(std::istream &is) override;
  explicit ConvexShape(Version version) : bhk::SphereRepShape(version) {}
  ~ConvexShape() override = 0;
};
inline ConvexShape::~ConvexShape() = default;

struct SphereShape : bhk::ConvexShape {
  void read(std::istream &is) override;
  explicit SphereShape(Version version) : ConvexShape(version) {}
};

struct CapsuleShape : bhk::ConvexShape {
  std::array<basic::Byte, 8> unused{};
  compound::Vector3 firstPoint{};
  basic::Float firstRadius{};
  compound::Vector3 secondPoint{};
  basic::Float secondRadius{};

  void read(std::istream &is) override;
  explicit CapsuleShape(Version version) : bhk::ConvexShape(version) {}
};

struct BoxShape : bhk::ConvexShape {
  std::array<basic::Byte, 8> unused{};
  // Stored in half-extents, so unit cube is {0.5f,0.5f,0.5f}
  compound::Vector4 dimensions{};

  void read(std::istream &is) override;
  explicit BoxShape(Version version) : bhk::ConvexShape(version) {}
};

struct ConvexVerticesShape : bhk::ConvexShape {
  compound::hk::WorldObjCinfoProperty verticesProperty{};
  compound::hk::WorldObjCinfoProperty normalsProperty{};

  basic::UInt numVertices{};
  std::vector<compound::Vector4> vertices{};

  // Each normal determines a half-space, with the first three components
  // pointing towards the exterior and the fourth component the signed distance
  // from the separating plane to the origin, i.e. -v.n with v on the plane.
  basic::UInt numNormals{};
  std::vector<compound::Vector4> normals{};

  void read(std::istream &is) override;
  explicit ConvexVerticesShape(Version version) : bhk::ConvexShape(version) {}
};

struct ConvexTransformShape : bhk::TransformShape {
  void read(std::istream &is) override;
  explicit ConvexTransformShape(Version version)
      : bhk::TransformShape(version) {}
};

struct ConvexSweepShape : bhk::Shape, Versionable {
  basic::Ref<bhk::Shape> shape{};
  compound::HavokMaterial material{version};
  basic::Float radius{};
  compound::Vector3 unknown{};

  void read(std::istream &is) override;
  explicit ConvexSweepShape(Version version) : Versionable(version) {}
};

struct BvTreeShape : bhk::Shape {
  void read(std::istream &is) override;
  ~BvTreeShape() override = 0;
};
inline BvTreeShape::~BvTreeShape() = default;

// MOPP = Memory Optimized Partial Polytope
struct MoppBvTreeShape : bhk::BvTreeShape, Versionable {
  basic::Ref<bhk::Shape> shape{};
  compound::HavokMaterial material{version};
  std::array<basic::UInt, 2> unused{};
  basic::Float shapeScale{1.0f};
  // Calculated
  basic::UInt moppDataSize{};

  // Minimum of all vertices in the packed shape along each axis, minus 0.1
  VersionOptional<compound::Vector3, "10.1.0.0"_ver, Unbounded> origin{version};

  // Quantization factor is 2^16 / scale. Should be 2^16 * 254 / (size + 0.2),
  // with size the largest dimension of the bbox of the packed shape.
  VersionOptional<basic::Float, "10.1.0.0"_ver, Unbounded> scale{version};

  std::vector<basic::Byte> moppData{};

  void read(std::istream &is) override;
  explicit MoppBvTreeShape(Version version) : Versionable(version) {}
};

struct ShapeCollection : bhk::Shape {
  void read(std::istream &is) override;
  ~ShapeCollection() override = 0;
};
inline ShapeCollection::~ShapeCollection() = default;

struct ListShape : bhk::ShapeCollection, Versionable {
  basic::UInt numSubShapes{};
  std::vector<basic::Ref<bhk::Shape>> subShapes{};
  compound::HavokMaterial material{version};
  compound::hk::WorldObjCinfoProperty childShapeProperty{};
  compound::hk::WorldObjCinfoProperty childFilterProperty{};
  basic::UInt numUnknownInts{};
  std::vector<basic::UInt> unknownInts{};

  void read(std::istream &is) override;
  explicit ListShape(Version version) : Versionable(version) {}
};

struct PackedNiTriStripsShape : bhk::ShapeCollection, Versionable {
  basic::UShort numSubShapes{};
  std::vector<compound::OblivionSubShape> subShapes{};

  basic::UInt userData{};
  basic::UInt unused1{};
  basic::Float radius{0.1f};
  basic::UInt unused2{};
  compound::Vector4 scale{1.0f, 1.0f, 1.0f, 0.0f};
  basic::Float radiusCopy{radius};
  compound::Vector4 scaleCopy{scale};

  basic::Ref<nif::hk::PackedNiTriStripsData> data{};

  void read(std::istream &is) override;
  explicit PackedNiTriStripsShape(Version version) : Versionable(version) {}
};

struct WorldObject : bhk::Serializable, Versionable {
  basic::Ref<bhk::Shape> shape{};
  VersionOptional<basic::UInt, Unbounded, "10.0.1.2"_ver> unknownInt{version};
  compound::HavokFilter havokFilter{};
  std::array<basic::Byte, 4> unused1{};
  Enum::BroadPhaseType broadPhaseType{Enum::BroadPhaseType::BROAD_PHASE_ENTITY};
  std::array<basic::Byte, 3> unused2{};
  compound::hk::WorldObjCinfoProperty cinfoProperty{};

  void read(std::istream &is) override;
  explicit WorldObject(Version version) : Versionable(version) {}
  ~WorldObject() override = 0;
};
inline WorldObject::~WorldObject() = default;

struct Phantom : bhk::WorldObject {
  void read(std::istream &is) override;
  explicit Phantom(Version version) : bhk::WorldObject(version) {}
  ~Phantom() override = 0;
};
inline Phantom::~Phantom() = default;

struct ShapePhantom : bhk::Phantom {
  void read(std::istream &is) override;
  explicit ShapePhantom(Version version) : bhk::Phantom(version) {}
  ~ShapePhantom() override = 0;
};
inline ShapePhantom::~ShapePhantom() = default;

struct SimpleShapePhantom : bhk::ShapePhantom {
  std::array<basic::Byte, 8> unused3{};
  compound::Matrix44 transform{};

  void read(std::istream &is) override;
  explicit SimpleShapePhantom(Version version) : bhk::ShapePhantom(version) {}
};

struct Entity : bhk::WorldObject {
  void read(std::istream &is) override;
  explicit Entity(Version version) : bhk::WorldObject(version) {}
  ~Entity() override = 0;
};
inline Entity::~Entity() = default;

struct Constraint : bhk::Serializable {
  basic::UInt numEntities{};
  std::vector<basic::Ptr<bhk::Entity>> entities{};
  basic::UInt priority{};

  void read(std::istream &is) override;
  ~Constraint() override = 0;
};
inline Constraint::~Constraint() = default;

struct LimitedHingeConstraint : bhk::Constraint {
  compound::LimitedHingeDescriptor descriptor{};

  void read(std::istream &is) override;
};

struct RagdollConstraint : bhk::Constraint {
  compound::RagdollDescriptor descriptor{};

  void read(std::istream &is) override;
};

// Ignores rotation and translation
struct RigidBody : bhk::Entity {
  Enum::hk::ResponseType
      collisionResponse{Enum::hk::ResponseType::RESPONSE_SIMPLE_CONTACT};
  basic::Byte unusedByte1{};

  // Callback is raised every processContactCallbackDelay frames
  basic::UShort processContactCallbackDelay{0xffff};

  VersionOptional<basic::UInt, "10.1.0.0"_ver, Unbounded> unknownInt1{version};

  VersionOptional<compound::HavokFilter, "10.1.0.0"_ver, Unbounded>
      havokFilterCopy{version};

  VersionOptional<std::array<basic::Byte, 4>, "10.1.0.0"_ver, Unbounded>
      unused2{version};

  VersionOptional<Enum::hk::ResponseType, "10.1.0.0"_ver, Unbounded>
      collisionResponse2
      {version, Enum::hk::ResponseType::RESPONSE_SIMPLE_CONTACT};

  VersionOptional<basic::Byte, "10.1.0.0"_ver, Unbounded> unusedByte2{version};

  VersionOptional<basic::UShort, "10.1.0.0"_ver, Unbounded>
      processContactCallbackDelay2{version, 0xffff};

  // userVer2 <= 34
  basic::UInt unknownInt2{};

  compound::Vector4 translation{};
  compound::hk::Quaternion rotation{};
  compound::Vector4 linearVelocity{};
  compound::Vector4 angularVelocity{};
  compound::hk::Matrix3 inertiaTensor{};
  compound::Vector4 center{};
  // Zero is immovable (kg)
  basic::Float mass{1.0f};
  // 0.1f = remove 10% of linear velocity per second
  basic::Float linearDamping{0.1f};
  // 0.05f = remove 5% of angular velocity per second
  basic::Float angularDamping{0.05f};
  basic::Float friction{0.5f};
  basic::Float restitution{0.4f};

  VersionOptional<basic::Float, "10.1.0.0"_ver, Unbounded>
      maxLinearVelocity{version, 104.4f};

  VersionOptional<basic::Float, "10.1.0.0"_ver, Unbounded>
      maxAngularVelocity{version, 31.57f};

  // userVer2 != 130
  VersionOptional<basic::Float, "10.1.0.0"_ver, Unbounded>
      penetrationDepth{version, 0.15f};

  Enum::hk::MotionType motionSystem{Enum::hk::MotionType::MO_SYS_DYNAMIC};
  // userVer2 <= 34
  Enum::hk::DeactivatorType
      deactivatorType{Enum::hk::DeactivatorType::DEACTIVATOR_NEVER};
  Enum::hk::SolverDeactivation
      solverDeactivation{Enum::hk::SolverDeactivation::SOLVER_DEACTIVATION_OFF};
  Enum::hk::QualityType qualityType{Enum::hk::QualityType::MO_QUAL_FIXED};

  std::array<basic::Byte, 12> unknownBytes1{};

  basic::UInt numConstraints{};
  std::vector<basic::Ref<bhk::Serializable>> constraints{};

  // 1 = respond to wind
  basic::UInt bodyFlags{};

  void read(std::istream &is) override;
  explicit RigidBody(Version version) : Entity(version) {}
};

// Doesn't ignore rotation and translation
struct RigidBodyT : bhk::RigidBody {
  void read(std::istream &is) override;
  explicit RigidBodyT(Version version) : bhk::RigidBody(version) {}
};

struct NiCollisionObject : nif::NiCollisionObject {
  Enum::bhk::COFlags flags{}; // = 1
  basic::Ref<bhk::WorldObject> body{};

  void read(std::istream &is) override;
  ~NiCollisionObject() override = 0;
};
inline NiCollisionObject::~NiCollisionObject() = default;

struct CollisionObject : bhk::NiCollisionObject {
  void read(std::istream &is) override;
};

struct BlendController : nif::NiTimeController {
  basic::UInt keys{};
  void read(std::istream &is) override;
};

struct BlendCollisionObject : bhk::NiCollisionObject {
  basic::Float heirGain{};
  basic::Float velGain{};
  void read(std::istream &is) override;
};

} // namespace bhk

namespace hk {

struct PackedNiTriStripsData : bhk::ShapeCollection {
  basic::UInt numTriangles{};
  std::vector<compound::TriangleData> triangles{};

  basic::UInt numVertices{};
  std::vector<compound::Vector3> vertices{};

  void read(std::istream &is) override;
};

} // namespace hk

} // namespace nif

#endif // OPENOBLIVION_NIF_NIOBJECT_HPP
