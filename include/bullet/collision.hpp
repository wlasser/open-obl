#ifndef OPENOBLIVION_BULLET_COLLISION_HPP
#define OPENOBLIVION_BULLET_COLLISION_HPP

#include <btBulletDynamicsCommon.h>
#include <gsl/gsl>
#include <functional>

namespace bullet {

gsl::span<const btPersistentManifold *const>
inline getManifolds(gsl::not_null<btCollisionDispatcher *> dispatcher) {
  const int numManifolds = dispatcher->getNumManifolds();
  const btPersistentManifold *const *manifolds =
      dispatcher->getInternalManifoldPointer();
  return gsl::make_span(manifolds, numManifolds);
}

// Use this to be notified of collisions involving a target btCollisionObject.
// Register one or more callbacks to a (pointer to a) btCollisionObject and call
// runCallbacks once each frame to have the callbacks called whenever the
// registered btCollisionObjects are involved in a collision. The callback
// should receive as arguments a pointer to the other btCollisionObject
// involved in the collision, and the point at which the collision occurs. If
// the same two objects collide in multiple points, only one point is counted.
class CollisionCaller {
 public:
  using callback_t = std::function<void(const btCollisionObject *,
                                        const btManifoldPoint &)>;

 private:
  std::unordered_map<const btCollisionObject *, std::vector<callback_t>> mMap{};

  void dispatch(const btCollisionObject *a, const btCollisionObject *b,
                const btManifoldPoint &contact) {
    const auto it = mMap.find(a);
    if (it != std::end(mMap)) {
      for (const auto &callback : it->second) {
        std::invoke(callback, b, contact);
      }
    }
  }

 public:

  void runCallbacks(gsl::not_null<btCollisionDispatcher *> dispatcher) {
    for (const auto *manifold : getManifolds(dispatcher)) {
      // We only want one contact point, and only dispatch each once
      if (manifold->getNumContacts() > 0) {
        const auto &contact = manifold->getContactPoint(0);
        if (contact.getLifeTime() < 2) {
          const auto *first = manifold->getBody0();
          const auto *second = manifold->getBody1();
          dispatch(first, second, contact);
        }
      }
    }
  }

  void addCallback(const btCollisionObject *target,
                   const callback_t &callback) {
    mMap[target].push_back(callback);
  }
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
                               btBroadphaseProxy *proxy1) const override {
    const auto &rawGroup0{reinterpret_cast<const uint32_t &>(
                              proxy0->m_collisionFilterGroup)};
    const auto &rawGroup1{reinterpret_cast<const uint32_t &>(
                              proxy1->m_collisionFilterGroup)};
    const auto &rawMask0{reinterpret_cast<const uint32_t &>(
                             proxy0->m_collisionFilterMask)};
    const auto &rawMask1{reinterpret_cast<const uint32_t &>(
                             proxy1->m_collisionFilterMask)};

    const uint64_t mask0{(static_cast<uint64_t>(rawMask0) << 26u)
                             | (static_cast<uint64_t>(rawGroup0) >> 6u)};
    const uint64_t mask1{(static_cast<uint64_t>(rawMask1) << 26u)
                             | (static_cast<uint64_t>(rawGroup1) >> 6u)};
    const uint64_t group0{1u << (static_cast<uint64_t>(rawGroup0) & 0b111111u)};
    const uint64_t group1{1u << (static_cast<uint64_t>(rawGroup1) & 0b111111u)};

    return (group0 & mask1) && (group1 & mask0);
  }

  /// Take a `group` \f$\in [0, 57]\f$ and a 58-bit `mask` and pack them
  /// into an `int` group and mask suitable for `LayeredOverlapFilter`.
  /// \returns A packed `{mask, group}` pair.
  template<class Group = int, class Mask = uint64_t,
      class = std::enable_if_t<
          (std::is_integral_v<Group> && std::is_unsigned_v<Mask>) ||
              (std::is_same_v<Group, Mask> && std::is_enum_v<Mask> &&
                  std::is_unsigned_v<std::underlying_type_t<Mask>>)>>
  static constexpr std::pair<int, int>
  makeFilter(Group group, Mask mask) noexcept {
    return std::pair<int, int>(
        static_cast<Mask>(group) | (mask & ((1u << 26u) - 1u) << 6u),
        mask >> 26u
    );
  }

  /// Wrapper around `btCollisionWorld::addCollisionObject` that calls
  /// `LayeredOverlapFilter::makeFilter` on its `group` and `mask` arguments.
  template<class Group = int, class Mask = uint64_t,
      class = std::enable_if_t<
          (std::is_integral_v<Group> && std::is_unsigned_v<Mask>) ||
              (std::is_same_v<Group, Mask> && std::is_enum_v<Mask> &&
                  std::is_unsigned_v<std::underlying_type_t<Mask>>)>>
  static void addCollisionObject(btCollisionWorld *world,
                                 btCollisionObject *collisionObject,
                                 Group group,
                                 Mask mask) {
    const auto[g, m]{LayeredOverlapFilter::makeFilter(group, mask)};
    world->addCollisionObject(collisionObject, g, m);
  }

  /// Wrapper around `btDynamicsWorld::addRigidBody` that calls
  /// `LayeredOverlapFilter::makeFilter` on its `group` and `mask` arguments.
  template<class Group = int, class Mask = uint64_t,
      class = std::enable_if_t<
          (std::is_integral_v<Group> && std::is_unsigned_v<Mask>) ||
              (std::is_same_v<Group, Mask> && std::is_enum_v<Mask> &&
                  std::is_unsigned_v<std::underlying_type_t<Mask>>)>>
  static void addRigidBody(btDynamicsWorld *world,
                           btRigidBody *body,
                           Group group,
                           Mask mask) {
    const auto[g, m]{LayeredOverlapFilter::makeFilter(group, mask)};
    world->addRigidBody(body, g, m);
  }
};

} // namespace bullet

#endif // OPENOBLIVION_BULLET_COLLISION_HPP
