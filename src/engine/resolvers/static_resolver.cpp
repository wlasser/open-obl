#include "engine/resolvers/resolvers.hpp"
#include "engine/resolvers/static_resolver.hpp"

namespace engine {

auto Resolver<record::STAT>::make(BaseId baseId,
                                  gsl::not_null<Ogre::SceneManager *> mgr,
                                  std::optional<RefId> id) const -> make_t {
  const auto entry{mMap.find(baseId)};
  if (entry == mMap.end()) return {};
  const store_t &rec{entry->second};

  auto *const entity{loadMesh(rec, mgr)};
  auto *const rigidBody{loadRigidBody(entity, mgr)};

  if (rigidBody) {
    // TODO: Get a new RefId properly
    const RefId refId{id ? *id : RefId{}};
    setRefId(gsl::make_not_null(rigidBody), refId);
  }

  return {ecs::RigidBody{rigidBody}, ecs::Mesh{entity}};
}

bool Resolver<record::STAT>::add(BaseId baseId, store_t entry) {
  return mMap.try_emplace(baseId, entry).second;
}

bool Resolver<record::STAT>::contains(BaseId baseId) const noexcept {
  return mMap.find(baseId) != mMap.end();
}

} // namespace engine