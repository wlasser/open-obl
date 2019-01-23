#include "fs/path.hpp"
#include "record/records.hpp"
#include "resolvers/acti_resolver.hpp"
#include "resolvers/helpers.hpp"

namespace oo {

template<> oo::CiteRecordTrait<record::ACTI>::type
citeRecord(const record::ACTI &baseRec, tl::optional<RefId> refId) {
  record::REFR_ACTI::Raw rawRefRec{};
  rawRefRec.baseId = record::NAME(BaseId{baseRec.mFormId});
  const record::REFR_ACTI refRec(rawRefRec,
                                 record::RecordFlag::None,
      // TODO: Get a new RefId properly
                                 static_cast<oo::FormId>(refId ? *refId
                                                               : oo::RefId{}),
                                 0);
  return refRec;
}

template<> oo::ReifyRecordTrait<record::REFR_ACTI>::type
reifyRecord(const record::REFR_ACTI &refRec,
            gsl::not_null<Ogre::SceneManager *> scnMgr,
            oo::ReifyRecordTrait<record::REFR_ACTI>::resolvers resolvers) {
  const auto &actiRes{oo::getResolver<record::ACTI>(resolvers)};
  auto baseRec{actiRes.get(refRec.baseId.data)};
  if (!baseRec) return {ecs::RigidBody{nullptr}, ecs::Mesh{nullptr}};

  Ogre::Entity *mesh{oo::loadMesh(*baseRec, scnMgr)};
  Ogre::RigidBody *rigidBody{oo::loadRigidBody(mesh, scnMgr)};

  if (rigidBody) {
    setRefId(gsl::make_not_null(rigidBody), oo::RefId{refRec.mFormId});
  }

  return {ecs::RigidBody{rigidBody}, ecs::Mesh{mesh}};
}

} // namespace oo