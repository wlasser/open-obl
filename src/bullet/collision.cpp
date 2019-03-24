#include "bullet/collision.hpp"
#include "record/formid.hpp"
#include "settings.hpp"
#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>

namespace bullet {

gsl::span<const btPersistentManifold *const>
getManifolds(gsl::not_null<btCollisionDispatcher *> dispatcher) {
  const int numManifolds{dispatcher->getNumManifolds()};
  const btPersistentManifold *const *
      manifolds{dispatcher->getInternalManifoldPointer()};
  return gsl::make_span(manifolds, numManifolds);
}

void CollisionCaller::dispatch(const btCollisionObject *a,
                               const btCollisionObject *b,
                               const btManifoldPoint &contact) {
  if (auto it{mMap.find(a)}; it != std::end(mMap)) {
    for (const auto &callback : it->second) {
      std::invoke(callback, b, contact);
    }
  }
}

void CollisionCaller::runCallbacks(gsl::not_null<btCollisionDispatcher *> dispatcher) {
  for (const auto *manifold : bullet::getManifolds(dispatcher)) {
    // We only want one contact point, and only dispatch each once
    if (manifold->getNumContacts() > 0) {
      const auto &contact{manifold->getContactPoint(0)};
      if (contact.getLifeTime() < 2) {
        dispatch(manifold->getBody0(), manifold->getBody1(), contact);
      }
    }
  }
}

void CollisionCaller::addCallback(const btCollisionObject *target,
                                  const callback_t &callback) {
  mMap[target].push_back(callback);
}

bool
LayeredOverlapFilter::needBroadphaseCollision(btBroadphaseProxy *proxy0,
                                              btBroadphaseProxy *proxy1) const {
  constexpr auto cast64 = [](const int &x) -> uint64_t {
    return static_cast<uint64_t>(reinterpret_cast<const uint32_t &>(x));
  };

  const auto g0{cast64(proxy0->m_collisionFilterGroup)};
  const auto g1{cast64(proxy1->m_collisionFilterGroup)};
  const auto m0{cast64(proxy0->m_collisionFilterMask)};
  const auto m1{cast64(proxy1->m_collisionFilterMask)};

  const uint64_t mask0{(m0 << 26ull) | (g0 >> 6ull)};
  const uint64_t mask1{(m1 << 26ull) | (g1 >> 6ull)};
  const uint64_t group0{1ull << (g0 & 0b111111ull)};
  const uint64_t group1{1ull << (g1 & 0b111111ull)};

  return (group0 & mask1) && (group1 & mask0);
}

void setCollisionLayer(gsl::not_null<Ogre::CollisionShape *> shape,
                       CollisionLayer layer) noexcept {
  const auto[group, mask]{bullet::getCollisionFilter(layer)};
  shape->setCollisionGroup(group);
  shape->setCollisionMask(mask);
}

void addRigidBody(gsl::not_null<btDynamicsWorld *> world,
                  gsl::not_null<Ogre::RigidBody *> body) {
  world->addRigidBody(body->getRigidBody(),
                      body->getCollisionGroup(),
                      body->getCollisionMask());
}

void removeRigidBody(gsl::not_null<btDynamicsWorld *> world,
                     gsl::not_null<Ogre::RigidBody *> body) {
  world->removeRigidBody(body->getRigidBody());
}

void reportingNearCallback(btBroadphasePair &collisionPair,
                           btCollisionDispatcher &dispatcher,
                           const btDispatcherInfo &dispatchInfo) {
  auto obj0{static_cast<btCollisionObject *>(
                collisionPair.m_pProxy0->m_clientObject)};
  auto obj1{static_cast<btCollisionObject *>(
                collisionPair.m_pProxy1->m_clientObject)};

  auto group0{collisionPair.m_pProxy0->m_collisionFilterGroup & 0b111111ull};
  auto group1{collisionPair.m_pProxy1->m_collisionFilterGroup & 0b111111ull};

  oo::FormId refId0{oo::decodeFormId(obj0->getUserPointer())};
  oo::FormId refId1{oo::decodeFormId(obj1->getUserPointer())};

  spdlog::get(oo::LOG)->trace("Collision Pair 0x{:0>8x} ({}) vs 0x{:0>8x} ({})",
                              refId0, group0, refId1, group1);

  btCollisionDispatcher::defaultNearCallback(collisionPair,
                                             dispatcher,
                                             dispatchInfo);
}

} // namespace bullet