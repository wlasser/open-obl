#include "record/records.hpp"
#include "resolvers/door_resolver.hpp"
#include "resolvers/helpers.hpp"

namespace oo {

auto
CiteRecordImpl<record::DOOR>::operator()(const record::DOOR &baseRec,
                                         tl::optional<RefId> refId) -> type {
  record::REFR_DOOR::Raw rawRefRec{};
  rawRefRec.baseId = record::NAME(oo::BaseId{baseRec.mFormId});
  const record::REFR_DOOR refRec(rawRefRec,
                                 record::RecordFlag::None,
      // TODO: Get a new RefId properly
                                 static_cast<oo::FormId>(refId ? *refId
                                                               : oo::RefId{}),
                                 0);
  return refRec;
}

auto ReifyRecordImpl<record::REFR_DOOR>::operator()(
    const record::REFR_DOOR &refRec,
    Ogre::SceneManager *scnMgr,
    btDiscreteDynamicsWorld *world,
    resolvers res,
    Ogre::SceneNode *rootNode) -> type {
  const auto &doorRes{oo::getResolver<record::DOOR>(res)};
  auto baseRec{doorRes.get(refRec.baseId.data)};
  if (!baseRec || !baseRec->modelFilename) return nullptr;

  return oo::insertNif(*baseRec, oo::RefId{refRec.mFormId},
                       gsl::make_not_null(scnMgr), gsl::make_not_null(world),
                       rootNode);
}

} // namespace oo
