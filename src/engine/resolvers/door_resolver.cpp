#include "engine/resolvers/resolvers.hpp"
#include "engine/resolvers/door_resolver.hpp"

namespace engine {

DoorResolver::get_t
DoorResolver::get(BaseId baseId, Ogre::SceneManager *mgr) const {
  const auto entry{doors.find(baseId)};
  if (entry == doors.end()) return {};

  const store_t &rec{entry->second};

  auto *const entity{loadMesh(rec, mgr)};
  auto *const rigidBody{loadRigidBody(entity, mgr)};

  return {rigidBody, entity};
}

bool DoorResolver::add(BaseId baseId, store_t entry) {
  return doors.try_emplace(baseId, entry).second;
}

} // namespace engine
