#include "fs/path.hpp"
#include "resolvers/resolvers.hpp"
#include "resolvers/static_resolver.hpp"

auto Resolver<record::STAT>::make(BaseId baseId,
                                  gsl::not_null<Ogre::SceneManager *> mgr,
                                  std::optional<RefId> id) const -> make_t {
  const auto entry{mMap.find(baseId)};
  if (entry == mMap.end()) return {};
  const store_t &rec{entry->second};

  Ogre::Entity *entity = [&rec, mgr]() -> Ogre::Entity * {
    fs::Path rawPath{rec.modelFilename.data};
    std::string meshName{(fs::Path{"meshes"} / rawPath).c_str()};
    return loadMesh(meshName, mgr);
  }();

  Ogre::RigidBody *rigidBody{loadRigidBody(entity, mgr)};

  if (rigidBody) {
    // TODO: Get a new RefId properly
    const RefId refId{id ? *id : RefId{}};
    setRefId(gsl::make_not_null(rigidBody), refId);
  }

  return {ecs::RigidBody{rigidBody}, ecs::Mesh{entity}};
}

bool Resolver<record::STAT>::add(BaseId baseId, store_t entry) {
  return mMap.insert_or_assign(baseId, entry).second;
}

bool Resolver<record::STAT>::contains(BaseId baseId) const noexcept {
  return mMap.find(baseId) != mMap.end();
}
