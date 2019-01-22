#include "fs/path.hpp"
#include "record/records.hpp"
#include "resolvers/door_resolver.hpp"
#include "resolvers/helpers.hpp"
#include "resolvers/resolvers.hpp"

namespace oo {

template<> oo::CiteRecordTrait<record::DOOR>::type
citeRecord(const record::DOOR &baseRec, tl::optional<oo::RefId> refId) {
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

template<> oo::ReifyRecordTrait<record::REFR_DOOR>::type
reifyRecord(const record::REFR_DOOR &refRec,
            gsl::not_null<Ogre::SceneManager *> scnMgr,
            oo::ReifyRecordTrait<record::REFR_DOOR>::resolvers resolvers) {
  const auto &doorRes{oo::getResolver<record::DOOR>(resolvers)};
  auto baseRec{doorRes.get(refRec.baseId.data)};
  if (!baseRec) return {ecs::RigidBody{nullptr}, ecs::Mesh{nullptr}};

  Ogre::Entity *mesh{oo::loadMesh(*baseRec, scnMgr)};
  Ogre::RigidBody *rigidBody{oo::loadRigidBody(mesh, scnMgr)};

  if (rigidBody) {
    setRefId(gsl::make_not_null(rigidBody), oo::RefId{refRec.mFormId});
  }

  return {ecs::RigidBody{rigidBody}, ecs::Mesh{mesh}};
}

} // namespace oo
