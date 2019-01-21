#ifndef OPENOBLIVION_DOOR_RESOLVER_HPP
#define OPENOBLIVION_DOOR_RESOLVER_HPP

#include "record/records_fwd.hpp"
#include "record/reference_records.hpp"
#include "resolvers/ecs.hpp"
#include "resolvers/resolvers.hpp"
#include "ogrebullet/rigid_body.hpp"
#include <OgreEntity.h>
#include <string>

namespace oo {

using DoorResolver = Resolver<record::DOOR>;
using RefrDoorResolver = Resolver<record::REFR_DOOR, RefId>;

template<>
struct CiteRecordTrait<record::DOOR> {
  using type = record::REFR_DOOR;
};

template<>
struct ReifyRecordTrait<record::REFR_DOOR> {
  using type = ecs::Entity<ecs::RigidBody, ecs::Mesh>;
  using resolvers = std::tuple<const Resolver<record::DOOR> &>;
};

template<>
CiteRecordTrait<record::DOOR>::type
citeRecord(const record::DOOR &baseRec, tl::optional<RefId> refId);

template<>
ReifyRecordTrait<record::REFR_DOOR>::type
reifyRecord(const record::REFR_DOOR &refRec,
            gsl::not_null<Ogre::SceneManager *> scnMgr,
            ReifyRecordTrait<record::REFR_DOOR>::resolvers resolvers);

} // namespace oo

#endif // OPENOBLIVION_DOOR_RESOLVER_HPP
