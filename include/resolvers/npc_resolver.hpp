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
  using type = ecs::Entity<ecs::RigidBody<0>,
                           ecs::RigidBody<1>, ecs::Mesh<1>,
                           ecs::RigidBody<2>, ecs::Mesh<2>,
                           ecs::RigidBody<3>, ecs::Mesh<3>,
                           ecs::RigidBody<4>, ecs::Mesh<4>>;
  using resolvers = ResolverTuple<record::NPC_, record::RACE>;
};

template<> CiteRecordTrait<record::NPC_>::type
citeRecord(const record::NPC_ &baseRec, tl::optional<RefId> refId);

template<> ReifyRecordTrait<record::REFR_NPC_>::type
reifyRecord(const record::REFR_NPC_ &refRec,
            gsl::not_null<Ogre::SceneManager *> scnMgr,
            ReifyRecordTrait<record::REFR_NPC_>::resolvers resolvers);

} // namespace oo

#endif // OPENOBLIVION_NPC_RESOLVER_HPP
