#include "engine/resolvers/resolvers.hpp"
#include "engine/resolvers/door_resolver.hpp"

namespace engine {

DoorResolver::get_t
DoorResolver::get(FormID baseID, Ogre::SceneManager *mgr) const {
  const auto entry{doors.find(baseID)};
  if (entry == doors.end()) return {};

  const store_t &rec{entry->second};

  auto *const entity{loadMesh(rec, mgr)};
  auto *const rigidBody{loadRigidBody(entity, mgr)};

  return {rigidBody, entity};
}

bool DoorResolver::add(FormID baseID, store_t entry) {
  return doors.try_emplace(baseID, entry).second;
}

} // namespace engine
