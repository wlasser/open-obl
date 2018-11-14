#include "fs/path.hpp"
#include "resolvers/door_resolver.hpp"
#include "resolvers/resolvers.hpp"

template<>
CiteRecordTrait<record::DOOR>::type
citeRecord(const record::DOOR &baseRec, tl::optional<RefId> refId) {
  record::REFR_DOOR::Raw rawRefRec{};
  rawRefRec.baseId = record::NAME(BaseId{baseRec.mFormId});
  const record::REFR_DOOR refRec(rawRefRec,
                                 record::RecordFlag::None,
      // TODO: Get a new RefId properly
                                 static_cast<FormId>(refId ? *refId : RefId{}),
                                 0);
  return refRec;
}

template<>
ReifyRecordTrait<record::REFR_DOOR>::type
reifyRecord(const record::REFR_DOOR &refRec,
            gsl::not_null<Ogre::SceneManager *> scnMgr,
            ReifyRecordTrait<record::REFR_DOOR>::resolvers resolvers) {
  const auto &doorRes{std::get<const Resolver<record::DOOR> &>(resolvers)};
  auto baseRec{doorRes.get(refRec.baseId.data)};
  if (!baseRec) return {ecs::RigidBody{nullptr}, ecs::Mesh{nullptr}};

  Ogre::Entity *mesh{loadMesh(*baseRec, scnMgr)};
  Ogre::RigidBody *rigidBody{loadRigidBody(mesh, scnMgr)};

  if (rigidBody) {
    setRefId(gsl::make_not_null(rigidBody), RefId{refRec.mFormId});
  }

  return {ecs::RigidBody{rigidBody}, ecs::Mesh{mesh}};
}
