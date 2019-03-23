#ifndef OPENOBLIVION_DOOR_RESOLVER_HPP
#define OPENOBLIVION_DOOR_RESOLVER_HPP

#include "resolvers/resolvers.hpp"

namespace oo {

using DoorResolver = Resolver<record::DOOR>;
using RefrDoorResolver = Resolver<record::REFR_DOOR, RefId>;

template<>
struct CiteRecordTrait<record::DOOR> {
  using type = record::REFR_DOOR;
};

template<>
struct ReifyRecordTrait<record::REFR_DOOR> {
  using type = Ogre::SceneNode *;
  using resolvers = ResolverTuple<record::DOOR>;
};

template<>
CiteRecordTrait<record::DOOR>::type
citeRecord(const record::DOOR &baseRec, tl::optional<RefId> refId);

template<>
ReifyRecordTrait<record::REFR_DOOR>::type
reifyRecord(const record::REFR_DOOR &refRec,
            gsl::not_null<Ogre::SceneManager *> scnMgr,
            gsl::not_null<btDiscreteDynamicsWorld *> world,
            ReifyRecordTrait<record::REFR_DOOR>::resolvers resolvers,
            Ogre::SceneNode *rootNode);

} // namespace oo

#endif // OPENOBLIVION_DOOR_RESOLVER_HPP
