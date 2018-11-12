#include "fs/path.hpp"
#include "resolvers/door_resolver.hpp"
#include "resolvers/resolvers.hpp"

template<>
ReifyRecordTrait<record::DOOR>::type
reifyRecord(const record::DOOR &rec,
            gsl::not_null<Ogre::SceneManager *> scnMgr,
            tl::optional<RefId> refId) {
  Ogre::Entity *entity{loadMesh(rec, scnMgr)};
  Ogre::RigidBody *rigidBody{loadRigidBody(entity, scnMgr)};

  if (rigidBody) {
    // TODO: Get a new RefId properly
    setRefId(gsl::make_not_null(rigidBody), refId ? *refId : RefId{});
  }

  return {ecs::RigidBody{rigidBody}, ecs::Mesh{entity}};
}

