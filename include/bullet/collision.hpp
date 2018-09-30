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

} // namespace bullet

#endif // OPENOBLIVION_BULLET_COLLISION_HPP
