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
}

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
  std::array<basic::UInt, 3> unused{};
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
