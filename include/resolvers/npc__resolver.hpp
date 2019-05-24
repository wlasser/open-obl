#ifndef OPENOBL_NPC__RESOLVER_HPP
#define OPENOBL_NPC__RESOLVER_HPP

#include "character_controller/character.hpp"
#include "resolvers/resolvers.hpp"

namespace oo {

using Npc_Resolver = Resolver<record::NPC_>;
using RefrNpc_Resolver = Resolver<record::REFR_NPC_, RefId>;

template<> struct CiteRecordTrait<record::NPC_> {
  using type = record::REFR_NPC_;
};

template<> struct ReifyRecordTrait<record::REFR_NPC_> {
  using type = std::unique_ptr<oo::Character>;
  using resolvers = ResolverTuple<record::NPC_, record::RACE>;
};

template<> CiteRecordTrait<record::NPC_>::type
citeRecord(const record::NPC_ &baseRec, tl::optional<RefId> refId);

template<> ReifyRecordTrait<record::REFR_NPC_>::type
reifyRecord(const record::REFR_NPC_ &refRec,
            gsl::not_null<Ogre::SceneManager *> scnMgr,
            gsl::not_null<btDiscreteDynamicsWorld *> world,
            ReifyRecordTrait<record::REFR_NPC_>::resolvers resolvers,
            Ogre::SceneNode *rootNode);

} // namespace oo

#endif //OPENOBL_NPC__RESOLVER_HPP
