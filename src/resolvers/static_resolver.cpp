#include "fs/path.hpp"
#include "resolvers/resolvers.hpp"
#include "resolvers/static_resolver.hpp"

template<>
ReifyRecordTrait<record::STAT>::type
reifyRecord(const record::STAT &rec,
            gsl::not_null<Ogre::SceneManager *> scnMgr,
            tl::optional<RefId> refId) {
  Ogre::Entity *mesh{loadMesh(rec, scnMgr)};
  Ogre::RigidBody *rigidBody{loadRigidBody(mesh, scnMgr)};

  if (rigidBody) {
    // TODO: Get a new RefId properly
    setRefId(gsl::make_not_null(rigidBody), refId ? *refId : RefId{});
  }

  return {ecs::RigidBody{rigidBody}, ecs::Mesh{mesh}};
}
