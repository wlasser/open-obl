#ifndef OPENOBLIVION_BULLET_COLLISION_HPP
#define OPENOBLIVION_BULLET_COLLISION_HPP

#include "nif/enum.hpp"
#include "ogrebullet/collision_shape.hpp"
#include "ogrebullet/rigid_body.hpp"
#include <btBulletDynamicsCommon.h>
#include <gsl/gsl>
#include <functional>

namespace bullet {

gsl::span<const btPersistentManifold *const>
getManifolds(gsl::not_null<btCollisionDispatcher *> dispatcher);

/// Use this to be notified of collisions involving a target
/// `btCollisionObject`.
/// Register one or more callbacks to a (pointer to a) `btCollisionObject` and
/// call `runCallbacks1 once each frame to have the callbacks called whenever
/// the registered `btCollisionObject`s are involved in a collision.
/// The callback should receive as arguments a pointer to the other
/// `btCollisionObject` involved in the collision, and the point at which the
/// collision occurs. If the same two objects collide in multiple points, only
/// one point is counted.
class CollisionCaller {
 public:
  using callback_t = std::function<void(const btCollisionObject *,
                                        const btManifoldPoint &)>;

 private:
  std::unordered_map<const btCollisionObject *, std::vector<callback_t>> mMap{};

  void dispatch(const btCollisionObject *a, const btCollisionObject *b,
                const btManifoldPoint &contact);

 public:

  void runCallbacks(gsl::not_null<btCollisionDispatcher *> dispatcher);
  void addCallback(const btCollisionObject *target, const callback_t &callback);
};

/// Support a larger number of collision groups than normal by restricting each
/// object to a single group.
///
/// In Bullet's usual mask-based collision filtering, each object is assigned
/// one or more groups and a 32-bit mask. Say object \f$A\f$ has group
/// \f$A_G\f$ and mask \f$A_M\f$ and object \f$B\f$ has group \f$B_G\f$ and
/// mask \f$B_M\f$. Then \f$A\f$ *collides with* \f$B\f$ if
/// \f$(A_G \& B_M) \&\& (B_G \& A_M)\f$. This condition is symmetric, so
/// 'collides with' is a symmetric binary relation. Loosely, \f$A\f$ collides
/// with \f$B\f$ if at least one of \f$A_G\f$'s bits are set in \f$B_M\f$ and
/// at least one of \f$B_G\f$'s bits are set in \f$A_M\f$. In the simplest use
/// of this scheme the groups are powers of two, so there are a maximum of
/// 32 different groups. Putting an object in multiple groups, or adding
/// hierarchical groups, can be done by `|`-ing the groups together. This is
/// very powerful, but the cap of 32 disjoint groups may be limiting.
///
/// One method to squeeze more disjoint groups into Bullet's 32-bit mask and
/// 32-bit group is to assume that each object belongs to exactly one group.
/// Then, the group does not need to be a power of two---since groups no longer
/// need to be `|`-ed together---and can be an ordinary integer. The bottom 5
/// bits of the group suffice for a 32-bit mask. This leaves 27 bits in the
/// group free. Taking an additional bottom bit gives up to 64 different groups,
/// and leaves 26 bits free to act as additional mask bits. Those bits increase
/// the number of groups supported by the mask by 26, thus giving us 58
/// different collision groups.
class LayeredOverlapFilter : public btOverlapFilterCallback {
 public:
  bool needBroadphaseCollision(btBroadphaseProxy *proxy0,
                               btBroadphaseProxy *proxy1) const override;

  /// Take a `group` \f$\in [0, 57]\f$ and a 58-bit `mask` and pack them
  /// into an `int` group and mask suitable for `LayeredOverlapFilter`.
  /// \returns A packed `{group, mask}` pair.
  template<class Group = int, class Mask = uint64_t>
  static constexpr std::pair<int, int>
  makeFilter(Group group, Mask mask) noexcept {
    return std::pair<int, int>(
        static_cast<uint32_t>(static_cast<Mask>(group) | (mask << 6ull)),
        static_cast<uint32_t>(mask >> 26ull)
    );
  }
};

using CollisionLayer = nif::Enum::OblivionLayer;
using CollisionMaterial = nif::Enum::OblivionHavokMaterial;

template<CollisionLayer... Layers> constexpr uint64_t
getCollisionMaskImpl() noexcept {
  return (0x0ull | ... | (1ull << static_cast<uint64_t>(Layers)));
}

constexpr uint64_t getCollisionMask(CollisionLayer layer) noexcept;

/// \returns a Bullet-compatible `{group, mask}` pair.
constexpr std::pair<int, int>
getCollisionFilter(CollisionLayer layer) noexcept {
  return LayeredOverlapFilter::makeFilter(layer,
                                          bullet::getCollisionMask(layer));
}

void setCollisionLayer(gsl::not_null<Ogre::CollisionShape *> shape,
                       CollisionLayer layer) noexcept;

/// Wrapper around `btDynamicsWorld::addRigidBody` that respects the collision
/// group and mask of the given `body`.
void addRigidBody(gsl::not_null<btDynamicsWorld *> world,
                  gsl::not_null<Ogre::RigidBody *> body);

/// Wrapper around `btDynamicsWorld::removeRigidBody` for consistency with
/// `bullet::addRigidBody`.
void removeRigidBody(gsl::not_null<btDynamicsWorld *> world,
                     gsl::not_null<Ogre::RigidBody *> body);

//===----------------------------------------------------------------------===//
// Constexpr function definitions
//===----------------------------------------------------------------------===//

constexpr uint64_t getCollisionMask(CollisionLayer layer) noexcept {
  using Layer = CollisionLayer;
  constexpr auto BodyMask{bullet::getCollisionMaskImpl<
      Layer::OL_HEAD, Layer::OL_BODY, Layer::OL_SPINE1, Layer::OL_SPINE2,
      Layer::OL_L_UPPER_ARM, Layer::OL_L_FOREARM, Layer::OL_L_HAND,
      Layer::OL_L_THIGH, Layer::OL_L_CALF, Layer::OL_L_FOOT,
      Layer::OL_R_UPPER_ARM, Layer::OL_R_FOREARM, Layer::OL_R_HAND,
      Layer::OL_R_THIGH, Layer::OL_R_CALF, Layer::OL_R_FOOT>()};
  constexpr auto BodyColliders{bullet::getCollisionMaskImpl<
      Layer::OL_STATIC, Layer::OL_ANIM_STATIC, Layer::OL_WEAPON,
      Layer::OL_PROJECTILE>()};

  constexpr std::array<uint64_t, 64u> masks{
      0u, // OL_UNIDENTIFIED
      bullet::getCollisionMaskImpl< // OL_STATIC
          Layer::OL_CLUTTER, Layer::OL_WEAPON, Layer::OL_PROJECTILE,
          Layer::OL_SPELL, Layer::OL_BIPED, Layer::OL_TRAP,
          Layer::OL_CAMERA_PICK, Layer::OL_ITEM_PICK, Layer::OL_LINE_OF_SIGHT,
          Layer::OL_PATH_PICK>() | BodyMask,
      bullet::getCollisionMaskImpl< // OL_ANIM_STATIC
          Layer::OL_CLUTTER, Layer::OL_WEAPON, Layer::OL_PROJECTILE,
          Layer::OL_SPELL, Layer::OL_BIPED, Layer::OL_TRAP,
          Layer::OL_CAMERA_PICK, Layer::OL_ITEM_PICK, Layer::OL_LINE_OF_SIGHT,
          Layer::OL_PATH_PICK>() | BodyMask,
      bullet::getCollisionMaskImpl< // OL_TRANSPARENT
          Layer::OL_CLUTTER, Layer::OL_BIPED, Layer::OL_PATH_PICK>(),
      bullet::getCollisionMaskImpl< // OL_CLUTTER
          Layer::OL_STATIC, Layer::OL_ANIM_STATIC, Layer::OL_TRANSPARENT,
          Layer::OL_CLUTTER, Layer::OL_WEAPON, Layer::OL_PROJECTILE,
          Layer::OL_SPELL, Layer::OL_BIPED, Layer::OL_TREES,
          Layer::OL_WATER, Layer::OL_TRIGGER, Layer::OL_TERRAIN,
          Layer::OL_TRAP, Layer::OL_GROUND, Layer::OL_CAMERA_PICK,
          Layer::OL_ITEM_PICK, Layer::OL_SPELL_EXPLOSION>(),
      bullet::getCollisionMaskImpl< // OL_WEAPON
          Layer::OL_STATIC, Layer::OL_ANIM_STATIC, Layer::OL_CLUTTER,
          Layer::OL_WEAPON, Layer::OL_PROJECTILE, Layer::OL_SPELL,
          Layer::OL_BIPED, Layer::OL_TREES, Layer::OL_WATER,
          Layer::OL_TRIGGER, Layer::OL_TERRAIN, Layer::OL_TRAP,
          Layer::OL_GROUND, Layer::OL_CAMERA_PICK, Layer::OL_ITEM_PICK,
          Layer::OL_SPELL_EXPLOSION, Layer::OL_SHIELD>() | BodyMask,
      bullet::getCollisionMaskImpl< // OL_PROJECTILE
          Layer::OL_STATIC, Layer::OL_ANIM_STATIC, Layer::OL_CLUTTER,
          Layer::OL_WEAPON, Layer::OL_TREES, Layer::OL_WATER,
          Layer::OL_TRIGGER, Layer::OL_TERRAIN, Layer::OL_GROUND,
          Layer::OL_ITEM_PICK, Layer::OL_SHIELD>() | BodyMask,
      bullet::getCollisionMaskImpl< // OL_SPELL
          Layer::OL_STATIC, Layer::OL_ANIM_STATIC, Layer::OL_CLUTTER,
          Layer::OL_WEAPON, Layer::OL_BIPED, Layer::OL_TREES,
          Layer::OL_TRAP, Layer::OL_TRIGGER, Layer::OL_TERRAIN,
          Layer::OL_GROUND>(),
      bullet::getCollisionMaskImpl< // OL_BIPED
          Layer::OL_STATIC, Layer::OL_ANIM_STATIC, Layer::OL_TRANSPARENT,
          Layer::OL_CLUTTER, Layer::OL_WEAPON, Layer::OL_SPELL,
          Layer::OL_BIPED, Layer::OL_TREES, Layer::OL_WATER,
          Layer::OL_TRIGGER, Layer::OL_TERRAIN, Layer::OL_TRAP,
          Layer::OL_CLOUD_TRAP, Layer::OL_GROUND, Layer::OL_CAMERA_PICK,
          Layer::OL_LINE_OF_SIGHT, Layer::OL_PATH_PICK,
          Layer::OL_SPELL_EXPLOSION>(),
      bullet::getCollisionMaskImpl< // OL_TREES
          Layer::OL_CLUTTER, Layer::OL_WEAPON, Layer::OL_PROJECTILE,
          Layer::OL_SPELL, Layer::OL_BIPED, Layer::OL_TRAP,
          Layer::OL_LINE_OF_SIGHT, Layer::OL_PATH_PICK>(),
      0u, // OL_PROPS
      bullet::getCollisionMaskImpl< // OL_WATER
          Layer::OL_CLUTTER, Layer::OL_WEAPON, Layer::OL_PROJECTILE,
          Layer::OL_BIPED, Layer::OL_TRAP>(),
      bullet::getCollisionMaskImpl< // OL_TRIGGER
          Layer::OL_CLUTTER, Layer::OL_WEAPON, Layer::OL_PROJECTILE,
          Layer::OL_SPELL, Layer::OL_BIPED, Layer::OL_TRAP,
          Layer::OL_SPELL_EXPLOSION>(),
      bullet::getCollisionMaskImpl< // OL_TERRAIN
          Layer::OL_CLUTTER, Layer::OL_WEAPON, Layer::OL_PROJECTILE,
          Layer::OL_SPELL, Layer::OL_BIPED, Layer::OL_TRAP,
          Layer::OL_CAMERA_PICK, Layer::OL_ITEM_PICK, Layer::OL_LINE_OF_SIGHT,
          Layer::OL_PATH_PICK>(),
      bullet::getCollisionMaskImpl< // OL_TRAP
          Layer::OL_STATIC, Layer::OL_ANIM_STATIC, Layer::OL_CLUTTER,
          Layer::OL_WEAPON, Layer::OL_SPELL, Layer::OL_BIPED,
          Layer::OL_TREES, Layer::OL_WATER, Layer::OL_TRIGGER,
          Layer::OL_TERRAIN, Layer::OL_GROUND, Layer::OL_CAMERA_PICK,
          Layer::OL_ITEM_PICK, Layer::OL_LINE_OF_SIGHT, Layer::OL_PATH_PICK,
          Layer::OL_SPELL_EXPLOSION>(),
      0u, // OL_NONCOLLIDABLE
      bullet::getCollisionMaskImpl< // OL_CLOUD_TRAP
          Layer::OL_BIPED>(),
      bullet::getCollisionMaskImpl< // OL_GROUND
          Layer::OL_CLUTTER, Layer::OL_WEAPON, Layer::OL_PROJECTILE,
          Layer::OL_SPELL, Layer::OL_BIPED, Layer::OL_TRAP,
          Layer::OL_LINE_OF_SIGHT, Layer::OL_PATH_PICK>(),
      0u, // OL_PORTAL
      0u, // OL_STAIRS
      0u, // OL_CHAR_CONTROLLER
      0u, // OL_AVOID_BOX
      0u, // OL_UNKNOWN1
      0u, // OL_UNKNOWN2
      bullet::getCollisionMaskImpl< // OL_CAMERA_PICK
          Layer::OL_STATIC, Layer::OL_ANIM_STATIC, Layer::OL_CLUTTER,
          Layer::OL_WEAPON, Layer::OL_BIPED, Layer::OL_TERRAIN,
          Layer::OL_TRAP>(),
      bullet::getCollisionMaskImpl< // OL_ITEM_PICK
          Layer::OL_STATIC, Layer::OL_ANIM_STATIC, Layer::OL_CLUTTER,
          Layer::OL_WEAPON, Layer::OL_PROJECTILE, Layer::OL_TERRAIN,
          Layer::OL_TRAP>(),
      bullet::getCollisionMaskImpl< // OL_LINE_OF_SIGHT
          Layer::OL_STATIC, Layer::OL_ANIM_STATIC, Layer::OL_BIPED,
          Layer::OL_TREES, Layer::OL_TERRAIN, Layer::OL_TRAP,
          Layer::OL_GROUND>(),
      bullet::getCollisionMaskImpl< // OL_PATH_PICK
          Layer::OL_STATIC, Layer::OL_ANIM_STATIC, Layer::OL_TRANSPARENT,
          Layer::OL_BIPED, Layer::OL_TREES, Layer::OL_TERRAIN,
          Layer::OL_TRAP, Layer::OL_GROUND>(),
      0u, // OL_CUSTOM_PICK_1
      0u, // OL_CUSTOM_PICK_2
      bullet::getCollisionMaskImpl< // OL_SPELL_EXPLOSION
          Layer::OL_CLUTTER, Layer::OL_WEAPON, Layer::OL_BIPED,
          Layer::OL_TRIGGER, Layer::OL_TRAP>(),
      0u, // OL_DROPPING_PICK
      0u, // OL_OTHER
      BodyColliders | BodyMask, // OL_HEAD
      BodyColliders | BodyMask, // OL_BODY
      BodyColliders | BodyMask, // OL_SPINE1
      BodyColliders | BodyMask, // OL_SPINE2
      BodyColliders | BodyMask, // OL_L_UPPER_ARM
      BodyColliders | BodyMask, // OL_L_FOREARM
      BodyColliders | BodyMask, // OL_L_HAND
      BodyColliders | BodyMask, // OL_L_THIGH
      BodyColliders | BodyMask, // OL_L_CALF
      BodyColliders | BodyMask, // OL_L_FOOT
      BodyColliders | BodyMask, // OL_R_UPPER_ARM
      BodyColliders | BodyMask, // OL_R_FOREARM
      BodyColliders | BodyMask, // OL_R_HAND
      BodyColliders | BodyMask, // OL_R_THIGH
      BodyColliders | BodyMask, // OL_R_CALF
      BodyColliders | BodyMask, // OL_R_FOOT
      0u, // OL_TAIL
      0u, // OL_SIDE_WEAPON
      bullet::getCollisionMaskImpl< // OL_SHIELD
          Layer::OL_WEAPON, Layer::OL_PROJECTILE>(),
      0u, // OL_QUIVER
      0u, // OL_BACK_WEAPON
      0u, // OL_BACK_WEAPON2,
      0u, // OL_PONYTAIL,
      0u, // OL_WING,
      0u, // OL_NULL
  };

  // The masks array is an adjacency matrix for a undirected graph, where there
  // is an edge between A and B if A and B can collide. This static_assert
  // checks that the adjacency matrix is symmetric. If it isn't, it returns a
  // nonzero value encoding the position of the discrepancy, but at the moment
  // there's no way to communicate that via the static_assert, so if it fails
  // make getCollisionMask inline instead of constexpr and uncomment the logger
  // to find out where the error is.
  constexpr auto isSymmetric = [](std::array<uint64_t, 64u> masks) -> uint64_t {
    for (std::size_t i = 0ull; i < 64ull; ++i) {
      for (std::size_t j = 0ull; j < i; ++j) {
        const uint64_t b1{(masks[i] & (1ull << j)) >> j};
        const uint64_t b2{(masks[j] & (1ull << i)) >> i};
        if (b1 != b2) return (1u << 12u) | (i << 6u) | j;
      }
    }
    return 0ull;
  };
  static_assert(isSymmetric(masks) == 0ull);
//  spdlog::get(oo::LOG)->info("isSymmetric(masks) == {}", isSymmetric(masks));

  return masks[static_cast<std::size_t>(layer)];
}

} // namespace bullet

#endif // OPENOBLIVION_BULLET_COLLISION_HPP
