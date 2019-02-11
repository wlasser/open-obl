#include "fs/path.hpp"
#include "record/records.hpp"
#include "resolvers/helpers.hpp"
#include "resolvers/static_resolver.hpp"

namespace oo {

template<> oo::CiteRecordTrait<record::STAT>::type
citeRecord(const record::STAT &baseRec, tl::optional<RefId> refId) {
  record::REFR_STAT::Raw rawRefRec{};
  rawRefRec.baseId = record::NAME(oo::BaseId{baseRec.mFormId});
  const record::REFR_STAT refRec(rawRefRec,
                                 record::RecordFlag::None,
      // TODO: Get a new RefId properly
                                 static_cast<oo::FormId>(refId ? *refId
                                                               : oo::RefId{}),
                                 0);
  return refRec;
}

template<> oo::ReifyRecordTrait<record::REFR_STAT>::type
reifyRecord(const record::REFR_STAT &refRec,
            gsl::not_null<Ogre::SceneManager *> scnMgr,
            gsl::not_null<btDiscreteDynamicsWorld *> world,
            oo::ReifyRecordTrait<record::REFR_STAT>::resolvers resolvers) {
  const auto &statRes{oo::getResolver<record::STAT>(resolvers)};
  auto baseRec{statRes.get(refRec.baseId.data)};
  if (!baseRec) return nullptr;

  return oo::insertNif(*baseRec, oo::RefId{refRec.mFormId}, scnMgr, world);
}

} // namespace oo
