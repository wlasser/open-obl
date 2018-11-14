#include "fs/path.hpp"
#include "resolvers/resolvers.hpp"
#include "resolvers/static_resolver.hpp"

template<>
CiteRecordTrait<record::STAT>::type
citeRecord(const record::STAT &baseRec, tl::optional<RefId> refId) {
  record::REFR_STAT::Raw rawRefRec{};
  rawRefRec.baseId = record::NAME(BaseId{baseRec.mFormId});
  const record::REFR_STAT refRec(rawRefRec,
                                 record::RecordFlag::None,
      // TODO: Get a new RefId properly
                                 static_cast<FormId>(refId ? *refId : RefId{}),
                                 0);
  return refRec;
}

template<>
ReifyRecordTrait<record::REFR_STAT>::type
reifyRecord(const record::REFR_STAT &refRec,
            gsl::not_null<Ogre::SceneManager *> scnMgr,
            ReifyRecordTrait<record::REFR_STAT>::resolvers resolvers) {
  const auto &statRes{std::get<const Resolver<record::STAT> &>(resolvers)};
  auto baseRec{statRes.get(refRec.baseId.data)};
  if (!baseRec) return {ecs::RigidBody{nullptr}, ecs::Mesh{nullptr}};

  Ogre::Entity *mesh{loadMesh(*baseRec, scnMgr)};
  Ogre::RigidBody *rigidBody{loadRigidBody(mesh, scnMgr)};

  if (rigidBody) {
    setRefId(gsl::make_not_null(rigidBody), RefId{refRec.mFormId});
  }

  return {ecs::RigidBody{rigidBody}, ecs::Mesh{mesh}};
}
