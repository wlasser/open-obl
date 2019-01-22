#ifndef OPENOBLIVION_NPC_RESOLVER_HPP
#define OPENOBLIVION_NPC_RESOLVER_HPP

#include "record/records_fwd.hpp"
#include "record/reference_records.hpp"
#include "resolvers/ecs.hpp"
#include "resolvers/resolvers.hpp"

namespace oo {

using Npc_Resolver = Resolver<record::NPC_>;
using RefrNpc_Resolver = Resolver<record::REFR_NPC_, RefId>;

template<> struct CiteRecordTrait<record::NPC_> {
  using type = record::REFR_NPC_;
};

template<> struct ReifyRecordTrait<record::REFR_NPC_> {
  using type = ecs::Entity<ecs::Mesh, ecs::Skeleton>;
  using resolvers = std::tuple<const Resolver<record::NPC_> &>;
};

template<> CiteRecordTrait<record::NPC_>::type
citeRecord(const record::NPC_ &baseRec, tl::optional<RefId> refId);

template<> ReifyRecordTrait<record::REFR_NPC_>::type
reifyRecord(const record::REFR_NPC_ &refRec,
            gsl::not_null<Ogre::SceneManager *> scnMgr,
            ReifyRecordTrait<record::REFR_NPC_>::resolvers resolvers);

} // namespace oo

#endif // OPENOBLIVION_NPC_RESOLVER_HPP
