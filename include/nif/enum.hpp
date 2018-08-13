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

} // namespace Enum
} // namespace nif

#endif // OPENOBLIVION_NIF_ENUM_HPP
