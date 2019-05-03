#include "fs/path.hpp"
#include "record/records.hpp"
#include "resolvers/helpers.hpp"
#include "resolvers/cont_resolver.hpp"

namespace oo {

template<> oo::CiteRecordTrait<record::CONT>::type
citeRecord(const record::CONT &baseRec, tl::optional<RefId> refId) {
  record::REFR_CONT::Raw rawRefRec{};
  rawRefRec.baseId = record::NAME(oo::BaseId{baseRec.mFormId});
  const record::REFR_CONT refRec(rawRefRec,
                                 record::RecordFlag::None,
      // TODO: Get a new RefId properly
                                 static_cast<oo::FormId>(refId ? *refId
                                                               : oo::RefId{}),
                                 0);
  return refRec;
}

template<> oo::ReifyRecordTrait<record::REFR_CONT>::type
reifyRecord(const record::REFR_CONT &refRec,
            gsl::not_null<Ogre::SceneManager *> scnMgr,
            gsl::not_null<btDiscreteDynamicsWorld *> world,
            oo::ReifyRecordTrait<record::REFR_CONT>::resolvers resolvers,
            Ogre::SceneNode *rootNode) {
  const auto &statRes{oo::getResolver<record::CONT>(resolvers)};
  auto baseRec{statRes.get(refRec.baseId.data)};
  if (!baseRec) return nullptr;

  return oo::insertNif(*baseRec, oo::RefId{refRec.mFormId}, scnMgr, world,
                       rootNode);
}

} // namespace oo
