#include "record/records.hpp"
#include "resolvers/helpers.hpp"
#include "resolvers/misc_resolver.hpp"

namespace oo {

template<> CiteRecordTrait<record::MISC>::type
citeRecord(const record::MISC &baseRec, tl::optional<RefId> refId) {
  record::REFR_MISC rawRefRec{};
  rawRefRec.baseId = record::NAME(oo::BaseId{baseRec.mFormId});
  const record::REFR_MISC refRec(rawRefRec,
                                 record::RecordFlag::None,
      // TODO: Get a new RefId properly
                                 static_cast<oo::FormId>(refId ? *refId
                                                               : oo::RefId{}),
                                 0);
  return refRec;
}

template<> ReifyRecordTrait<record::REFR_MISC>::type
reifyRecord(const record::REFR_MISC &refRec,
            gsl::not_null<Ogre::SceneManager *> scnMgr,
            gsl::not_null<btDiscreteDynamicsWorld *> world,
            oo::ReifyRecordTrait<record::REFR_MISC>::resolvers resolvers,
            Ogre::SceneNode *rootNode) {
  const auto &miscRes{oo::getResolver<record::MISC>(resolvers)};
  auto baseRec{miscRes.get(refRec.baseId.data)};
  if (!baseRec) return nullptr;

  return oo::insertNif(*baseRec, oo::RefId{refRec.mFormId}, scnMgr, world,
                       rootNode);
}

} // namespace oo