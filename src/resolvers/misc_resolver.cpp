#include "record/records.hpp"
#include "resolvers/helpers.hpp"
#include "resolvers/misc_resolver.hpp"

namespace oo {

auto
CiteRecordImpl<record::MISC>::operator()(const record::MISC &baseRec,
                                         tl::optional<RefId> refId) -> type {
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

auto ReifyRecordImpl<record::REFR_MISC>::operator()(
    const record::REFR_MISC &refRec,
    Ogre::SceneManager *scnMgr,
    btDiscreteDynamicsWorld *world,
    resolvers res,
    Ogre::SceneNode *rootNode) -> type {
  const auto &miscRes{oo::getResolver<record::MISC>(res)};
  auto baseRec{miscRes.get(refRec.baseId.data)};
  if (!baseRec) return nullptr;

  return oo::insertNif(*baseRec, oo::RefId{refRec.mFormId},
                       gsl::make_not_null(scnMgr), gsl::make_not_null(world),
                       rootNode);
}

} // namespace oo