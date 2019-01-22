#include "fs/path.hpp"
#include "record/records.hpp"
#include "resolvers/npc_resolver.hpp"
#include "resolvers/helpers.hpp"
#include "settings.hpp"
#include <OgreSkeletonManager.h>

namespace oo {

template<> CiteRecordTrait<record::NPC_>::type
citeRecord(const record::NPC_ &baseRec, tl::optional<RefId> refId) {
  record::REFR_NPC_::Raw rawRefRec{};
  rawRefRec.baseId = record::NAME(BaseId{baseRec.mFormId});
  const record::REFR_NPC_ refRec(rawRefRec,
                                 record::RecordFlag::None,
      // TODO: Get a new RefId properly
                                 static_cast<FormId>(refId ? *refId : RefId{}),
                                 0);
  return refRec;
}

template<> ReifyRecordTrait<record::REFR_NPC_>::type
reifyRecord(const record::REFR_NPC_ &refRec,
            gsl::not_null<Ogre::SceneManager *> scnMgr,
            ReifyRecordTrait<record::REFR_NPC_>::resolvers resolvers) {
  const auto &npcRes{std::get<const oo::Resolver<record::NPC_> &>(resolvers)};
  auto baseRec{npcRes.get(refRec.baseId.data)};
  if (!baseRec || !baseRec->skeletonFilename) {
    return {ecs::Mesh{nullptr}, ecs::Skeleton{nullptr}};
  }

  oo::Path rawSkelPath{baseRec->skeletonFilename->data};
  std::string skelPath{(oo::Path{"meshes"} / rawSkelPath).c_str()};
  auto &skelMgr{Ogre::SkeletonManager::getSingleton()};
  Ogre::SkeletonPtr skelPtr{skelMgr.getByName(skelPath, oo::RESOURCE_GROUP)};
  skelPtr->load();

  return {ecs::Mesh{nullptr}, ecs::Skeleton{skelPtr.get()}};
}

} // namespace oo
