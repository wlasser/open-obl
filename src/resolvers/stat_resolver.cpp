#include "record/records.hpp"
#include "resolvers/helpers.hpp"
#include "resolvers/stat_resolver.hpp"

namespace oo {

auto
CiteRecordImpl<record::STAT>::operator()(const record::STAT &baseRec,
                                         tl::optional<RefId> refId) -> type {
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

auto ReifyRecordImpl<record::REFR_STAT>::operator()(
    const record::REFR_STAT &refRec,
    Ogre::SceneManager *scnMgr,
    btDiscreteDynamicsWorld *world,
    resolvers res,
    Ogre::SceneNode *rootNode) -> type {
  const auto &statRes{oo::getResolver<record::STAT>(res)};
  auto baseRec{statRes.get(refRec.baseId.data)};
  if (!baseRec) return nullptr;

  return oo::insertNif(*baseRec, oo::RefId{refRec.mFormId},
                       gsl::make_not_null(scnMgr), gsl::make_not_null(world),
                       rootNode);
}

} // namespace oo
