#ifndef OPENOBLIVION_NIF_BHK_HPP
#define OPENOBLIVION_NIF_BHK_HPP

#include "nif/basic.hpp"
#include "nif/compound.hpp"
#include "nif/niobject.hpp"
#include "nif/versionable.hpp"
#include <array>
#include <istream>
#include <vector>

namespace nif::hk {
struct PackedNiTriStripsData;

namespace Enum {

enum class ConstraintType : uint32_t {
  BallAndSocket = 0,
  Hinge = 1,
  LimitedHinge = 2,
  Prismatic = 6,
  Ragdoll = 7,
  StiffSpring = 8,
  Malleable = 13
};

enum class DeactivatorType : uint8_t {
  DEACTIVATOR_INVALID = 0,
  DEACTIVATOR_NEVER = 1,
  DEACTIVATOR_SPATIAL = 2
};

enum class MotionType : uint8_t {
  MO_SYS_INVALID = 0,
  MO_SYS_DYNAMIC = 1,
  MO_SYS_SPHERE_INERTIA = 2,
  MO_SYS_SPHERE_STABILIZED = 3,
  MO_SYS_BOX_INERTIA = 4,
  MO_SYS_BOX_STABILIZED = 5,
  // Infinite mass when viewed by the rest of the system
      MO_SYS_KEYFRAMED = 6,
  MO_SYS_FIXED = 7,
  MO_SYS_THIN_BOX = 8,
  MO_SYS_CHARACTER = 9
};

// MO_QUAL_FIXED and MO_QUAL_KEYFRAMED cannot interact
// MO_QUAL_DEBRIS can interpenetrate but responds to bullets
// MO_QUAL_CRITICAL cannot interpenetrate
// MO_QUAL_MOVING can interpenetrate with MO_QUAL_MOVING and MO_QUAL_DEBRIS
enum class QualityType : uint8_t {
  MO_QUAL_INVALID = 0,
  MO_QUAL_FIXED = 1,
  MO_QUAL_KEYFRAMED = 2,
  MO_QUAL_DEBRIS = 3,
  MO_QUAL_MOVING = 4,
  MO_QUAL_CRITICAL = 5,
  MO_QUAL_BULLET = 6,
  MO_QUAL_USER = 7,
  MO_QUAL_CHARACTER = 8,
  MO_QUAL_KEYFRAMED_REPORT = 9
};

enum class ResponseType : uint8_t {
  RESPONSE_INVALID = 0,
  RESPONSE_SIMPLE_CONTACT = 1,
  RESPONSE_REPORTING = 2,
  RESPONSE_NONE = 3
};

enum class SolverDeactivation : uint8_t {
  SOLVER_DEACTIVATION_INVALID = 0,
  SOLVER_DEACTIVATION_OFF = 1,
  SOLVER_DEACTIVATION_LOW = 2,
  SOLVER_DEACTIVATION_MEDIUM = 3,
  SOLVER_DEACTIVATION_HIGH = 4,
  SOLVER_DEACTIVATION_MAX = 5
};
} // namespace Enum

} // namespace nif::hk

namespace nif::bhk {
namespace Enum {

enum class COFlags : uint16_t {
  NONE = 0,
  BHKCO_ACTIVE = 1u << 1u,
  BHKCO_NOTIFY = 1u << 2u,
  BHKCO_SET_LOCAL = 1u << 3u,
  BHKCO_DEBUG_DISPLAY = 1u << 4u,
  BHKCO_USE_VEL = 1u << 5u,
  BHKCO_RESET = 1u << 6u,
  BHKCO_SYNC_ON_UPDATE = 1u << 7u,
  BHKCO_ANIM_TARGETED = 1u << 10u,
  BHKCO_DISMEMBERED_LIMB = 1u << 11u
};
inline constexpr COFlags operator|(COFlags a, COFlags b) {
  return COFlags(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
}
inline constexpr COFlags operator&(COFlags a, COFlags b) {
  return COFlags(static_cast<uint16_t>(a) & static_cast<uint16_t>(b));
}

} // namespace Enum

struct RefObject : nif::NiObject {
  void read(std::istream &is) override;
  ~RefObject() override = 0;
};
inline RefObject::~RefObject() = default;

struct Serializable : RefObject {
  void read(std::istream &is) override;
  ~Serializable() override = 0;
};
inline Serializable::~Serializable() = default;

struct Shape : Serializable {
  void read(std::istream &is) override;
  ~Shape() override = 0;
};
inline Shape::~Shape() = default;

struct TransformShape : Shape, Versionable {
  basic::Ref<Shape> shape{};
  compound::HavokMaterial material{version};
  basic::Float radius{};
  std::array<basic::Byte, 8> unused{};
  compound::Matrix44 transform{};

  void read(std::istream &is) override;
  explicit TransformShape(Version version) : Versionable(version) {}
};

struct SphereRepShape : Shape, Versionable {
  compound::HavokMaterial material{version};
  basic::Float radius{};

  void read(std::istream &is) override;
  explicit SphereRepShape(Version version) : Versionable(version) {}
  ~SphereRepShape() override = 0;
};
inline SphereRepShape::~SphereRepShape() = default;

struct ConvexShape : SphereRepShape {
  void read(std::istream &is) override;
  explicit ConvexShape(Version version) : SphereRepShape(version) {}
  ~ConvexShape() override = 0;
};
inline ConvexShape::~ConvexShape() = default;

struct SphereShape : ConvexShape {
  void read(std::istream &is) override;
  explicit SphereShape(Version version) : ConvexShape(version) {}
};

struct CapsuleShape : ConvexShape {
  std::array<basic::Byte, 8> unused{};
  compound::Vector3 firstPoint{};
  basic::Float firstRadius{};
  compound::Vector3 secondPoint{};
  basic::Float secondRadius{};

  void read(std::istream &is) override;
  explicit CapsuleShape(Version version) : ConvexShape(version) {}
};

struct BoxShape : ConvexShape {
  std::array<basic::Byte, 8> unused{};
  // Stored in half-extents, so unit cube is {0.5f,0.5f,0.5f}
  compound::Vector4 dimensions{};

  void read(std::istream &is) override;
  explicit BoxShape(Version version) : ConvexShape(version) {}
};

struct ConvexVerticesShape : ConvexShape {
  compound::hkWorldObjCinfoProperty verticesProperty{};
  compound::hkWorldObjCinfoProperty normalsProperty{};

  basic::UInt numVertices{};
  std::vector<compound::Vector4> vertices{};

  // Each normal determines a half-space, with the first three components
  // pointing towards the exterior and the fourth component the signed distance
  // from the separating plane to the origin, i.e. -v.n with v on the plane.
  basic::UInt numNormals{};
  std::vector<compound::Vector4> normals{};

  void read(std::istream &is) override;
  explicit ConvexVerticesShape(Version version) : ConvexShape(version) {}
};

struct ConvexTransformShape : TransformShape {
  void read(std::istream &is) override;
  explicit ConvexTransformShape(Version version) : TransformShape(version) {}
};

struct ConvexSweepShape : Shape, Versionable {
  basic::Ref<Shape> shape{};
  compound::HavokMaterial material{version};
  basic::Float radius{};
  compound::Vector3 unknown{};

  void read(std::istream &is) override;
  explicit ConvexSweepShape(Version version) : Versionable(version) {}
};

struct BvTreeShape : Shape {
  void read(std::istream &is) override;
  ~BvTreeShape() override = 0;
};
inline BvTreeShape::~BvTreeShape() = default;

// MOPP = Memory Optimized Partial Polytope
struct MoppBvTreeShape : BvTreeShape, Versionable {
  basic::Ref<Shape> shape{};
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

struct ShapeCollection : Shape {
  void read(std::istream &is) override;
  ~ShapeCollection() override = 0;
};
inline ShapeCollection::~ShapeCollection() = default;

struct PackedNiTriStripsShape : ShapeCollection, Versionable {
  basic::UShort numSubShapes{};
  std::vector<nif::compound::OblivionSubShape> subShapes{};

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

struct WorldObject : Serializable, Versionable {
  basic::Ref<Shape> shape{};
  VersionOptional<basic::UInt, Unbounded, "10.0.1.2"_ver> unknownInt{version};
  compound::HavokFilter havokFilter{};
  std::array<basic::Byte, 4> unused1{};
  nif::Enum::BroadPhaseType
      broadPhaseType{nif::Enum::BroadPhaseType::BROAD_PHASE_ENTITY};
  std::array<basic::Byte, 3> unused2{};
  compound::hkWorldObjCinfoProperty cinfoProperty{};

  void read(std::istream &is) override;
  explicit WorldObject(Version version) : Versionable(version) {}
  ~WorldObject() override = 0;
};
inline WorldObject::~WorldObject() = default;

struct Phantom : WorldObject {
  void read(std::istream &is) override;
  explicit Phantom(Version version) : WorldObject(version) {}
  ~Phantom() override = 0;
};
inline Phantom::~Phantom() = default;

struct ShapePhantom : Phantom {
  void read(std::istream &is) override;
  explicit ShapePhantom(Version version) : Phantom(version) {}
  ~ShapePhantom() override = 0;
};
inline ShapePhantom::~ShapePhantom() = default;

struct SimpleShapePhantom : ShapePhantom {
  std::array<basic::Byte, 8> unused3{};
  compound::Matrix44 transform{};

  void read(std::istream &is) override;
  explicit SimpleShapePhantom(Version version) : ShapePhantom(version) {}
};

struct Entity : WorldObject {
  void read(std::istream &is) override;
  explicit Entity(Version version) : WorldObject(version) {}
  ~Entity() override = 0;
};
inline Entity::~Entity() = default;

// Ignores rotation and translation
struct RigidBody : Entity {
  hk::Enum::ResponseType
      collisionResponse{hk::Enum::ResponseType::RESPONSE_SIMPLE_CONTACT};
  basic::Byte unusedByte1{};

  // Callback is raised every processContactCallbackDelay frames
  basic::UShort processContactCallbackDelay{0xffff};

  VersionOptional<basic::UInt, "10.1.0.0"_ver, Unbounded> unknownInt1{version};

  VersionOptional<compound::HavokFilter, "10.1.0.0"_ver, Unbounded>
      havokFilterCopy{version};

  VersionOptional<std::array<basic::Byte, 4>, "10.1.0.0"_ver, Unbounded>
      unused2{version};

  VersionOptional<hk::Enum::ResponseType, "10.1.0.0"_ver, Unbounded>
      collisionResponse2
      {version, hk::Enum::ResponseType::RESPONSE_SIMPLE_CONTACT};

  VersionOptional<basic::Byte, "10.1.0.0"_ver, Unbounded> unusedByte2{version};

  VersionOptional<basic::UShort, "10.1.0.0"_ver, Unbounded>
      processContactCallbackDelay2{version, 0xffff};

  // userVer2 <= 34
  basic::UInt unknownInt2{};

  compound::Vector4 translation{};
  compound::hkQuaternion rotation{};
  compound::Vector4 linearVelocity{};
  compound::Vector4 angularVelocity{};
  compound::hkMatrix3 inertiaTensor{};
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

  hk::Enum::MotionType motionSystem{hk::Enum::MotionType::MO_SYS_DYNAMIC};
  // userVer2 <= 34
  hk::Enum::DeactivatorType
      deactivatorType{hk::Enum::DeactivatorType::DEACTIVATOR_NEVER};
  hk::Enum::SolverDeactivation
      solverDeactivation{hk::Enum::SolverDeactivation::SOLVER_DEACTIVATION_OFF};
  hk::Enum::QualityType qualityType{hk::Enum::QualityType::MO_QUAL_FIXED};

  std::array<basic::Byte, 12> unknownBytes1{};

  basic::UInt numConstraints{};
  std::vector<basic::Ref < Serializable>> constraints{};

  // 1 = respond to wind
  basic::UInt bodyFlags{};

  void read(std::istream &is) override;
  explicit RigidBody(Version version) : Entity(version) {}
};

// Doesn't ignore rotation and translation
struct RigidBodyT : RigidBody {
  void read(std::istream &is) override;
  explicit RigidBodyT(Version version) : RigidBody(version) {}
};

struct NiCollisionObject : nif::NiCollisionObject {
  Enum::COFlags flags{}; // = 1
  basic::Ref<WorldObject> body{};

  void read(std::istream &is) override;
  ~NiCollisionObject() override = 0;
};
inline NiCollisionObject::~NiCollisionObject() = default;

struct CollisionObject : NiCollisionObject {
  void read(std::istream &is) override;
};

} // namespace nif::bhk

namespace nif::hk {

struct PackedNiTriStripsData : nif::bhk::ShapeCollection {
  basic::UInt numTriangles{};
  std::vector<nif::compound::TriangleData> triangles{};

  basic::UInt numVertices{};
  std::vector<compound::Vector3> vertices{};

  void read(std::istream &is) override;
};
} // namespace nif::hk

#endif //OPENOBLIVION_NIF_BHK_HPP
