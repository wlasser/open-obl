#include "record/records.hpp"
#include "resolvers/acti_resolver.hpp"
#include "resolvers/helpers.hpp"

namespace oo {

auto
CiteRecordImpl<record::ACTI>::operator()(const record::ACTI &baseRec,
                                         tl::optional<RefId> refId) -> type {
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

auto ReifyRecordImpl<record::REFR_ACTI>::operator()(
    const record::REFR_ACTI &refRec,
    Ogre::SceneManager *scnMgr,
    btDiscreteDynamicsWorld *world,
    resolvers res,
    Ogre::SceneNode *rootNode) -> type {
  const auto &actiRes{oo::getResolver<record::ACTI>(res)};
  auto baseRec{actiRes.get(refRec.baseId.data)};
  if (!baseRec || !baseRec->modelFilename) return nullptr;

  return oo::insertNif(*baseRec, oo::RefId{refRec.mFormId},
                       gsl::make_not_null(scnMgr), gsl::make_not_null(world),
                       rootNode);
}

} // namespace oo