#include "record/records.hpp"
#include "resolvers/flor_resolver.hpp"
#include "resolvers/helpers.hpp"

namespace oo {

auto
CiteRecordImpl<record::FLOR>::operator()(const record::FLOR &baseRec,
                                         tl::optional<RefId> refId) -> type {
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

auto ReifyRecordImpl<record::REFR_FLOR>::operator()(
    const record::REFR_FLOR &refRec,
    Ogre::SceneManager *scnMgr,
    btDiscreteDynamicsWorld *world,
    resolvers res,
    Ogre::SceneNode *rootNode) -> type {
  const auto &statRes{oo::getResolver<record::FLOR>(res)};
  auto baseRec{statRes.get(refRec.baseId.data)};
  if (!baseRec) return nullptr;

  return oo::insertNif(*baseRec, oo::RefId{refRec.mFormId},
                       gsl::make_not_null(scnMgr), gsl::make_not_null(world),
                       rootNode);
}

} // namespace oo
