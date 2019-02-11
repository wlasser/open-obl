#ifndef OPENOBLIVION_NPC_RESOLVER_HPP
#define OPENOBLIVION_NPC_RESOLVER_HPP

#include "resolvers/ecs.hpp"
#include "resolvers/resolvers.hpp"

namespace oo {

using Npc_Resolver = Resolver<record::NPC_>;
using RefrNpc_Resolver = Resolver<record::REFR_NPC_, RefId>;

template<> struct CiteRecordTrait<record::NPC_> {
  using type = record::REFR_NPC_;
};

template<> struct ReifyRecordTrait<record::REFR_NPC_> {
  using type = Ogre::SceneNode *;
  using resolvers = ResolverTuple<record::NPC_, record::RACE>;
};

template<> CiteRecordTrait<record::NPC_>::type
citeRecord(const record::NPC_ &baseRec, tl::optional<RefId> refId);

template<> ReifyRecordTrait<record::REFR_NPC_>::type
reifyRecord(const record::REFR_NPC_ &refRec,
            gsl::not_null<Ogre::SceneManager *> scnMgr,
            gsl::not_null<btDiscreteDynamicsWorld *> world,
            ReifyRecordTrait<record::REFR_NPC_>::resolvers resolvers);

} // namespace oo

#endif // OPENOBLIVION_NPC_RESOLVER_HPP
