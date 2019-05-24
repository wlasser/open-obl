#include "record/records.hpp"
#include "resolvers/npc__resolver.hpp"
#include "resolvers/helpers.hpp"

namespace oo {

auto
CiteRecordImpl<record::NPC_>::operator()(const record::NPC_ &baseRec,
                                         tl::optional<RefId> refId) -> type {
  record::REFR_NPC_::Raw rawRefRec{};
  rawRefRec.baseId = record::NAME(BaseId{baseRec.mFormId});
  const record::REFR_NPC_ refRec(rawRefRec,
                                 record::RecordFlag::None,
      // TODO: Get a new RefId properly
                                 static_cast<FormId>(refId ? *refId : RefId{}),
                                 0);
  return refRec;
}

auto ReifyRecordImpl<record::REFR_NPC_>::operator()(
    const record::REFR_NPC_ &refRec,
    Ogre::SceneManager *scnMgr,
    btDiscreteDynamicsWorld *world,
    resolvers res,
    Ogre::SceneNode *) -> type {
  return std::make_unique<oo::Character>(refRec,
                                         gsl::make_not_null(scnMgr),
                                         gsl::make_not_null(world),
                                         res);
}

} // namespace oo
