#include "engine/resolvers/resolvers.hpp"
#include "engine/resolvers/static_resolver.hpp"

engine::RigidBodyEntity
engine::StaticResolver::get(BaseId baseId, Ogre::SceneManager *mgr) const {
  const auto entry{statics.find(baseId)};
  if (entry == statics.end()) return {};

  const store_t &rec{entry->second};

  auto *const entity{loadMesh(rec, mgr)};
  auto *const rigidBody{loadRigidBody(entity, mgr)};

  return {rigidBody, entity};
}

bool engine::StaticResolver::add(BaseId baseId, store_t entry) {
  return statics.try_emplace(baseId, entry).second;
}