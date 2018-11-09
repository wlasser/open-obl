#include "fs/path.hpp"
#include "resolvers/door_resolver.hpp"
#include "resolvers/resolvers.hpp"

auto Resolver<record::DOOR>::make(BaseId baseId,
                                  gsl::not_null<Ogre::SceneManager *> mgr,
                                  std::optional<RefId> id) const -> make_t {
  const auto entry{mMap.find(baseId)};
  if (entry == mMap.end()) return {};
  const store_t &rec{entry->second};

  Ogre::Entity *entity = [&rec, mgr]() -> Ogre::Entity * {
    if (rec.modelFilename) {
      fs::Path rawPath{rec.modelFilename->data};
      std::string meshName{(fs::Path{"meshes"} / rawPath).c_str()};
      return loadMesh(meshName, mgr);
    } else return nullptr;
  }();

  Ogre::RigidBody *rigidBody{loadRigidBody(entity, mgr)};

  if (rigidBody) {
    // TODO: Get a new RefId properly
    const RefId refId{id ? *id : RefId{}};
    setRefId(gsl::make_not_null(rigidBody), refId);
  }

  return {ecs::RigidBody{rigidBody}, ecs::Mesh{entity}};
}

bool Resolver<record::DOOR>::add(BaseId baseId, store_t entry) {
  return mMap.insert_or_assign(baseId, entry).second;
}

bool Resolver<record::DOOR>::contains(BaseId baseId) const noexcept {
  return mMap.find(baseId) != mMap.end();
}
