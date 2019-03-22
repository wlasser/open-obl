#ifndef OPENOBLIVION_NIF_ENUM_HPP
#define OPENOBLIVION_NIF_ENUM_HPP

namespace nif::Enum {

// Material descriptor for a Havok shape
// This is HNAM_LTEX::MaterialType
enum class OblivionHavokMaterial : uint32_t {
  OB_HAV_MAT_STONE = 0u, // Stone
  OB_HAV_MAT_CLOTH, // Cloth
  OB_HAV_MAT_DIRT, // Dirt
  OB_HAV_MAT_GLASS, // Glass
  OB_HAV_MAT_GRASS, // Grass
  OB_HAV_MAT_METAL, // Metal
  OB_HAV_MAT_ORGANIC, // Organic
  OB_HAV_MAT_SKIN, // Skin
  OB_HAV_MAT_WATER, // Water
  OB_HAV_MAT_WOOD, // Wood
  OB_HAV_MAT_HEAVY_STONE, // Heavy Stone
  OB_HAV_MAT_HEAVY_METAL, // Heavy Metal
  OB_HAV_MAT_HEAVY_WOOD, // Heavy Wood
  OB_HAV_MAT_CHAIN, // Chain
  OB_HAV_MAT_SNOW, // Snow
  OB_HAV_MAT_STONE_STAIRS, // Stone Stairs
  OB_HAV_MAT_CLOTH_STAIRS, // Cloth Stairs
  OB_HAV_MAT_DIRT_STAIRS, // Dirt Stairs
  OB_HAV_MAT_GLASS_STAIRS, // Glass Stairs
  OB_HAV_MAT_GRASS_STAIRS, // Grass Stairs
  OB_HAV_MAT_METAL_STAIRS, // Metal Stairs
  OB_HAV_MAT_ORGANIC_STAIRS, // Organic Stairs
  OB_HAV_MAT_SKIN_STAIRS, // Skin Stairs
  OB_HAV_MAT_WATER_STAIRS, // Water Stairs
  OB_HAV_MAT_WOOD_STAIRS, // Wood Stairs
  OB_HAV_MAT_HEAVY_STONE_STAIRS, // Heavy Stone Stairs
  OB_HAV_MAT_HEAVY_METAL_STAIRS, // Heavy Metal Stairs
  OB_HAV_MAT_HEAVY_WOOD_STAIRS, // Heavy Wood Stairs
  OB_HAV_MAT_CHAIN_STAIRS, // Chain Stairs
  OB_HAV_MAT_SNOW_STAIRS, // Snow Stairs
  OB_HAV_MAT_ELEVATOR, // Elevator
  OB_HAV_MAT_RUBBER, // Rubber
};

// Collision body layer a body belongs to
enum class OblivionLayer : uint8_t {
  OL_UNIDENTIFIED = 0u, // Unidentified (white)
  OL_STATIC, // Static (red)
  OL_ANIM_STATIC, // AnimStatic (magenta)
  OL_TRANSPARENT, // Transparent (light pink)
  OL_CLUTTER, // Clutter (light blue)
  OL_WEAPON, // Weapon (orange)
  OL_PROJECTILE, // Projectile (light orange)
  OL_SPELL, // Spell (cyan)
  OL_BIPED, // Biped (green) Seems to apply to all creatures/NPCs
  OL_TREES, // Trees (light brown)
  OL_PROPS, // Props (magenta)
  OL_WATER, // Water (cyan)
  OL_TRIGGER, // Trigger (light grey)
  OL_TERRAIN, // Terrain (light yellow)
  OL_TRAP, // Trap (light grey)
  OL_NONCOLLIDABLE, // NonCollidable (white)
  OL_CLOUD_TRAP, // CloudTrap (greenish grey)
  OL_GROUND, // Ground (none)
  OL_PORTAL, // Portal (green)
  OL_STAIRS, // Stairs (white)
  OL_CHAR_CONTROLLER, // CharController (yellow)
  OL_AVOID_BOX, // AvoidBox (dark yellow)
  OL_UNKNOWN1, // ? (white)
  OL_UNKNOWN2, // ? (white)
  OL_CAMERA_PICK, // CameraPick (white)
  OL_ITEM_PICK, // ItemPick (white)
  OL_LINE_OF_SIGHT, // LineOfSight (white)
  OL_PATH_PICK, // PathPick (white)
  OL_CUSTOM_PICK_1, // CustomPick1 (white)
  OL_CUSTOM_PICK_2, // CustomPick2 (white)
  OL_SPELL_EXPLOSION, // SpellExplosion (white)
  OL_DROPPING_PICK, // DroppingPick (white)
  OL_OTHER, // Other (white)
  OL_HEAD, // Head
  OL_BODY, // Body
  OL_SPINE1, // Spine1
  OL_SPINE2, // Spine2
  OL_L_UPPER_ARM, // LUpperArm
  OL_L_FOREARM, // LForeArm
  OL_L_HAND, // LHand
  OL_L_THIGH, // LThigh
  OL_L_CALF, // LCalf
  OL_L_FOOT, // LFoot
  OL_R_UPPER_ARM, // RUpperArm
  OL_R_FOREARM, // RForeArm
  OL_R_HAND, // RHand
  OL_R_THIGH, // RThigh
  OL_R_CALF, // RCalf
  OL_R_FOOT, // RFoot
  OL_TAIL, // Tail
  OL_SIDE_WEAPON, // SideWeapon
  OL_SHIELD, // Shield
  OL_QUIVER, // Quiver
  OL_BACK_WEAPON, // BackWeapon
  OL_BACK_WEAPON2, // BackWeapon (?)
  OL_PONYTAIL, // PonyTail
  OL_WING, // Wing
  OL_NULL, // Null
};

enum class EndianType : uint8_t {
  ENDIAN_BIG = 0,
  ENDIAN_LITTLE = 1u
};

enum class BoundVolumeType : uint32_t {
  BASE_BV = 0xffffffffu,
  SPHERE_BV = 0u,
  BOX_BV = 1u,
  CAPSULE_BV = 2u,
  UNION_BV = 4u,
  HALFSPACE_BV = 5u
};

enum class VectorFlags : uint16_t {
  NONE = 0u,
  VF_UV_1 = 1u << 0u,
  VF_UV_2 = 1u << 1u,
  VF_UV_4 = 1u << 2u,
  VF_UV_8 = 1u << 3u,
  VF_UV_16 = 1u << 4u,
  VF_UV_32 = 1u << 5u,
  VF_Unk64 = 1u << 6u,
  VF_Unk128 = 1u << 7u,
  VF_Unk256 = 1u << 8u,
  VF_Unk512 = 1u << 9u,
  VF_Unk1024 = 1u << 10u,
  VF_Unk2048 = 1u << 11u,
  VF_Has_Tangents = 1u << 12u,
  VF_Unk8192 = 1u << 13u,
  VF_Unk16384 = 1u << 14u,
  VF_Unk32768 = 1u << 15u,
  VF_UV_MASK = VF_UV_1 | VF_UV_2 | VF_UV_4 | VF_UV_8 | VF_UV_16 | VF_UV_32
};
inline constexpr VectorFlags operator|(VectorFlags a, VectorFlags b) {
  return VectorFlags(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
}
inline constexpr VectorFlags operator&(VectorFlags a, VectorFlags b) {
  return VectorFlags(static_cast<uint16_t>(a) & static_cast<uint16_t>(b));
}

// Controls volatility of mesh. Lower 12 bits are used at runtime.
enum class ConsistencyType : uint16_t {
  CT_MUTABLE = 0x0000u,
  CT_STATIC = 0x4000u,
  CT_VOLATILE = 0x8000u,
  mask = 0b1111000000000000u
};
inline constexpr ConsistencyType operator|(ConsistencyType a,
                                           ConsistencyType b) {
  return ConsistencyType(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
}
inline constexpr ConsistencyType operator&(ConsistencyType a,
                                           ConsistencyType b) {
  return ConsistencyType(static_cast<uint16_t>(a) & static_cast<uint16_t>(b));
}

// Vertex color blend mode with filtered texture color.
enum class ApplyMode : uint32_t {
  APPLY_REPLACE = 0u,
  APPLY_DECAL = 1u,
  APPLY_MODULATE = 2u,
  APPLY_HILIGHT = 3u,
  APPLY_HILIGHT2 = 4u
};

enum class TexClampMode : uint32_t {
  CLAMP_S_CLAMP_T = 0u,
  CLAMP_S_WRAP_T = 1u,
  WRAP_S_CLAMP_T = 2u,
  WRAP_S_WRAP_T = 3u
};

enum class TexFilterMode : uint32_t {
  FILTER_NEAREST = 0u,
  FILTER_BILERP = 1u,
  FILTER_TRILERP = 2u,
  FILTER_NEAREST_MIPNEAREST = 3u,
  FILTER_NEAREST_MIPLERP = 4u,
  FILTER_BILERP_MIPNEAREST = 5u,
  FILTER_ANISOTROPIC = 6u
};

// Order of application of transformation matrices.
// From parent TexDesc:
// T = translation, S = scale, R = rotation, C = center
// Others:
// F = inverse of v axis with a positive translation of 1 unit along v.
enum class TransformMethod : uint32_t {
  // C * R * C^(-1) * T * S
      MayaDeprecated = 0u,
  // C * S * R * T * C^(-1)
      Max = 1u,
  // C * R * C^(-1) * F * T * S
      Maya = 2u
};

enum class PixelLayout : uint32_t {
  PXLAY_PALETTIZED_8 = 0u,
  PXLAY_HIGH_COLOR_16 = 1u,
  PXLAY_TRUE_COLOR_32 = 2u,
  PXLAY_COMPRESSED = 3u,
  PXLAY_BUMPMAP = 4u,
  PXLAY_PALETTIZED_4 = 5u,
  PXLAY_DEFAULT = 6u,
  PXLAY_SINGLE_COLOR_8 = 7u,
  PXLAY_SINGLE_COLOR_16 = 8u,
  PXLAY_SINGLE_COLOR_32 = 9u,
  PXLAY_DOUBLE_COLOR_32 = 10u,
  PXLAY_DOUBLE_COLOR_64 = 11u,
  PXLAY_FLOAT_COLOR_32 = 12u,
  PXLAY_FLOAT_COLOR_64 = 13u,
  PXLAY_FLOAT_COLOR_128 = 14u,
  PXLAY_SINGLE_COLOR_4 = 15u,
  PXLAY_DEPTH_24_X8 = 16u
};

enum class MipMapFormat : uint32_t {
  MIP_FMT_NO = 0u,
  MIP_FMT_YES = 1u,
  MIP_FMT_DEFAULT = 2u
};

enum class AlphaFormat : uint32_t {
  ALPHA_NONE = 0u,
  ALPHA_BINARY,
  ALPHA_SMOOTH,
  ALPHA_DEFAULT
};

enum class BroadPhaseType : uint8_t {
  BROAD_PHASE_INVALID = 0u,
  BROAD_PHASE_ENTITY = 1u,
  BROAD_PHASE_PHANTOM = 2u,
  BROAD_PHASE_BORDER = 3u
};

// Test ref op buf with op given by enum code
enum class StencilCompareMode : uint32_t {
  TEST_NEVER = 0u, // Always test false, ignore ref
  TEST_LESS,
  TEST_EQUAL,
  TEST_LESS_EQUAL,
  TEST_GREATER,
  TEST_NOT_EQUAL,
  TEST_GREATER_EQUAL,
  TEST_ALWAYS // Always test true, ignore buf
};

enum class StencilAction : uint32_t {
  ACTION_KEEP = 0u,
  ACTION_ZERO,
  ACTION_REPLACE,
  ACTION_INCREMENT,
  ACTION_DECREMENT,
  ACTION_INVERT
};

enum class StencilDrawMode : uint32_t {
  DRAW_CCW_OR_BOTH = 0u,
  DRAW_CCW,
  DRAW_CW,
  DRAW_BOTH
};

enum class VertMode : uint32_t {
  VERT_MODE_SRC_IGNORE = 0u,
  VERT_MODE_SRC_EMISSIVE = 1u,
  VERT_MODE_SRC_AMB_DIF = 2u
};

enum class LightMode : uint32_t {
  LIGHT_MODE_EMISSIVE = 0u,
  LIGHT_MODE_EMI_AMB_DIF = 1u
};

enum class CycleType : uint32_t {
  CYCLE_LOOP = 0u,
  CYCLE_REVERSE = 1u,
  CYCLE_CLAMP = 2u
};

enum class KeyType : uint32_t {
  NONE = 0u,
  LINEAR_KEY = 1u,
  QUADRATIC_KEY = 2u,
  TBC_KEY = 3u,
  XYZ_ROTATION_KEY = 4u,
  CONST_KEY = 5u
};

enum class InterpBlendFlags : uint8_t {
  NONE = 0u,
  MANAGER_CONTROLLED = 1u
};

enum class MaterialColor : uint16_t {
  TC_AMBIENT = 0u,
  TC_DIFFUSE = 1u,
  TC_SPECULAR = 2u,
  TC_SELF_ILLUM = 3u
};

namespace hk {

enum class ConstraintType : uint32_t {
  BallAndSocket = 0u,
  Hinge = 1u,
  LimitedHinge = 2u,
  Prismatic = 6u,
  Ragdoll = 7u,
  StiffSpring = 8u,
  Malleable = 13u
};

enum class DeactivatorType : uint8_t {
  DEACTIVATOR_INVALID = 0u,
  DEACTIVATOR_NEVER = 1u,
  DEACTIVATOR_SPATIAL = 2u
};

enum class MotionType : uint8_t {
  MO_SYS_INVALID = 0u,
  MO_SYS_DYNAMIC = 1u,
  MO_SYS_SPHERE_INERTIA = 2u,
  MO_SYS_SPHERE_STABILIZED = 3u,
  MO_SYS_BOX_INERTIA = 4u,
  MO_SYS_BOX_STABILIZED = 5u,
  // Infinite mass when viewed by the rest of the system
      MO_SYS_KEYFRAMED = 6u,
  MO_SYS_FIXED = 7u,
  MO_SYS_THIN_BOX = 8u,
  MO_SYS_CHARACTER = 9u
};

// MO_QUAL_FIXED and MO_QUAL_KEYFRAMED cannot interact
// MO_QUAL_DEBRIS can interpenetrate but responds to bullets
// MO_QUAL_CRITICAL cannot interpenetrate
// MO_QUAL_MOVING can interpenetrate with MO_QUAL_MOVING and MO_QUAL_DEBRIS
enum class QualityType : uint8_t {
  MO_QUAL_INVALID = 0u,
  MO_QUAL_FIXED = 1u,
  MO_QUAL_KEYFRAMED = 2u,
  MO_QUAL_DEBRIS = 3u,
  MO_QUAL_MOVING = 4u,
  MO_QUAL_CRITICAL = 5u,
  MO_QUAL_BULLET = 6u,
  MO_QUAL_USER = 7u,
  MO_QUAL_CHARACTER = 8u,
  MO_QUAL_KEYFRAMED_REPORT = 9u
};

enum class ResponseType : uint8_t {
  RESPONSE_INVALID = 0u,
  RESPONSE_SIMPLE_CONTACT = 1u,
  RESPONSE_REPORTING = 2u,
  RESPONSE_NONE = 3u
};

enum class SolverDeactivation : uint8_t {
  SOLVER_DEACTIVATION_INVALID = 0u,
  SOLVER_DEACTIVATION_OFF = 1u,
  SOLVER_DEACTIVATION_LOW = 2u,
  SOLVER_DEACTIVATION_MEDIUM = 3u,
  SOLVER_DEACTIVATION_HIGH = 4u,
  SOLVER_DEACTIVATION_MAX = 5u
};

} // namespace hk

namespace bhk {

enum class COFlags : uint16_t {
  NONE = 0u,
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

} // namespace bhk

} // namespace nif::Enum

#endif // OPENOBLIVION_NIF_ENUM_HPP
