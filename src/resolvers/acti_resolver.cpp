#include "fs/path.hpp"
#include "resolvers/acti_resolver.hpp"
#include "resolvers/resolvers.hpp"

template<> CiteRecordTrait<record::ACTI>::type
citeRecord(const record::ACTI &baseRec, tl::optional<RefId> refId) {
  record::REFR_ACTI::Raw rawRefRec{};
  rawRefRec.baseId = record::NAME(BaseId{baseRec.mFormId});
  const record::REFR_ACTI refRec(rawRefRec,
                                 record::RecordFlag::None,
      // TODO: Get a new RefId properly
                                 static_cast<FormId>(refId ? *refId : RefId{}),
                                 0);
  return refRec;
}

template<> ReifyRecordTrait<record::REFR_ACTI>::type
reifyRecord(const record::REFR_ACTI &refRec,
            gsl::not_null<Ogre::SceneManager *> scnMgr,
            ReifyRecordTrait<record::REFR_ACTI>::resolvers resolvers) {
  const auto &actiRes{std::get<const Resolver<record::ACTI> &>(resolvers)};
  auto baseRec{actiRes.get(refRec.baseId.data)};
  if (!baseRec) return {ecs::RigidBody{nullptr}, ecs::Mesh{nullptr}};

  Ogre::Entity *mesh{loadMesh(*baseRec, scnMgr)};
  Ogre::RigidBody *rigidBody{loadRigidBody(mesh, scnMgr)};

  if (rigidBody) {
    setRefId(gsl::make_not_null(rigidBody), RefId{refRec.mFormId});
  }

  return {ecs::RigidBody{rigidBody}, ecs::Mesh{mesh}};
}