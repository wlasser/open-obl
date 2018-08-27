#ifndef OPENOBLIVION_NIF_ENUM_HPP
#define OPENOBLIVION_NIF_ENUM_HPP

#include "nif/basic.hpp"

namespace nif {
namespace Enum {

// Material descriptor for a Havok shape
// This is HNAM_LTEX::MaterialType
enum class OblivionHavokMaterial : uint32_t {
  OB_HAV_MAT_STONE = 0, // Stone
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
  OL_UNIDENTIFIED, // Unidentified (white)
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
  ENDIAN_LITTLE = 1
};

enum class BoundVolumeType : uint32_t {
  BASE_BV = 0xffffffff,
  SPHERE_BV = 0,
  BOX_BV = 1,
  CAPSULE_BV = 2,
  UNION_BV = 4,
  HALFSPACE_BV = 5
};

enum class VectorFlags : uint16_t {
  NONE = 0x0000,
  VF_UV_1 = 0x0001,
  VF_UV_2 = 0x0002,
  VF_UV_4 = 0x0004,
  VF_UV_8 = 0x0008,
  VF_UV_16 = 0x0010,
  VF_UV_32 = 0x0020,
  VF_Unk64 = 0x0040,
  VF_Unk128 = 0x0080,
  VF_Unk256 = 0x0100,
  VF_Unk512 = 0x0200,
  VF_Unk1024 = 0x0400,
  VF_Unk2048 = 0x0800,
  VF_Has_Tangents = 0x1000,
  VF_Unk8192 = 0x2000,
  VF_Unk16384 = 0x4000,
  VF_Unk32768 = 0x8000,
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
  CT_MUTABLE = 0x0000,
  CT_STATIC = 0x4000,
  CT_VOLATILE = 0x8000,
  mask = 0b1111000000000000
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
  APPLY_REPLACE = 0,
  APPLY_DECAL = 1,
  APPLY_MODULATE = 2,
  APPLY_HILIGHT = 3,
  APPLY_HILIGHT2 = 4
};

enum class TexClampMode : uint32_t {
  CLAMP_S_CLAMP_T = 0,
  CLAMP_S_WRAP_T = 1,
  WRAP_S_CLAMP_T = 2,
  WRAP_S_WRAP_T = 3
};

enum class TexFilterMode : uint32_t {
  FILTER_NEAREST = 0,
  FILTER_BILERP = 1,
  FILTER_TRILERP = 2,
  FILTER_NEAREST_MIPNEAREST = 3,
  FILTER_NEAREST_MIPLERP = 4,
  FILTER_BILERP_MIPNEAREST = 5,
  FILTER_ANISOTROPIC = 6
};

// Order of application of transformation matrices.
// From parent TexDesc:
// T = translation, S = scale, R = rotation, C = center
// Others:
// F = inverse of v axis with a positive translation of 1 unit along v.
enum class TransformMethod : uint32_t {
  // C * R * C^(-1) * T * S
      MayaDeprecated = 0,
  // C * S * R * T * C^(-1)
      Max = 1,
  // C * R * C^(-1) * F * T * S
      Maya = 2
};

enum class PixelLayout : uint32_t {
  PXLAY_PALETTIZED_8 = 0,
  PXLAY_HIGH_COLOR_16 = 1,
  PXLAY_TRUE_COLOR_32 = 2,
  PXLAY_COMPRESSED = 3,
  PXLAY_BUMPMAP = 4,
  PXLAY_PALETTIZED_4 = 5,
  PXLAY_DEFAULT = 6,
  PXLAY_SINGLE_COLOR_8 = 7,
  PXLAY_SINGLE_COLOR_16 = 8,
  PXLAY_SINGLE_COLOR_32 = 9,
  PXLAY_DOUBLE_COLOR_32 = 10,
  PXLAY_DOUBLE_COLOR_64 = 11,
  PXLAY_FLOAT_COLOR_32 = 12,
  PXLAY_FLOAT_COLOR_64 = 13,
  PXLAY_FLOAT_COLOR_128 = 14,
  PXLAY_SINGLE_COLOR_4 = 15,
  PXLAY_DEPTH_24_X8 = 16
};

enum class MipMapFormat : uint32_t {
  MIP_FMT_NO = 0,
  MIP_FMT_YES = 1,
  MIP_FMT_DEFAULT = 2
};

enum class AlphaFormat : uint32_t {
  ALPHA_NONE,
  ALPHA_BINARY,
  ALPHA_SMOOTH,
  ALPHA_DEFAULT
};

} // namespace Enum
} // namespace nif

#endif // OPENOBLIVION_NIF_ENUM_HPP
