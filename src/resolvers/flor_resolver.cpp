#include "record/records.hpp"
#include "resolvers/flor_resolver.hpp"
#include "resolvers/helpers.hpp"

namespace oo {

template<> oo::CiteRecordTrait<record::FLOR>::type
citeRecord(const record::FLOR &baseRec, tl::optional<RefId> refId) {
  record::REFR_FLOR::Raw rawRefRec{};
  rawRefRec.baseId = record::NAME(oo::BaseId{baseRec.mFormId});
  const record::REFR_FLOR refRec(rawRefRec,
                                 record::RecordFlag::None,
      // TODO: Get a new RefId properly
                                 static_cast<oo::FormId>(refId ? *refId
                                                               : oo::RefId{}),
                                 0);
  return refRec;
}

template<> oo::ReifyRecordTrait<record::REFR_FLOR>::type
reifyRecord(const record::REFR_FLOR &refRec,
            gsl::not_null<Ogre::SceneManager *> scnMgr,
            gsl::not_null<btDiscreteDynamicsWorld *> world,
            oo::ReifyRecordTrait<record::REFR_FLOR>::resolvers resolvers,
            Ogre::SceneNode *rootNode) {
  const auto &statRes{oo::getResolver<record::FLOR>(resolvers)};
  auto baseRec{statRes.get(refRec.baseId.data)};
  if (!baseRec) return nullptr;

  return oo::insertNif(*baseRec, oo::RefId{refRec.mFormId}, scnMgr, world,
                       rootNode);
}

} // namespace oo
