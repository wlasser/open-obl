#ifndef OPENOBL_NPC__RESOLVER_HPP
#define OPENOBL_NPC__RESOLVER_HPP

#include "character_controller/character.hpp"
#include "resolvers/resolvers.hpp"

namespace oo {

using Npc_Resolver = Resolver<record::NPC_>;
using RefrNpc_Resolver = Resolver<record::REFR_NPC_, RefId>;

template<> struct CiteRecordImpl<record::NPC_> {
  using type = record::REFR_NPC_;
  type operator()(const record::NPC_ &baseRec, tl::optional<RefId> refId);
};

template<> struct ReifyRecordImpl<record::REFR_NPC_> {
  using type = std::unique_ptr<oo::Character>;
  using resolvers = ResolverTuple<record::NPC_, record::RACE>;
  type operator()(const record::REFR_NPC_ &refRec,
                  Ogre::SceneManager *scnMgr,
                  btDiscreteDynamicsWorld *world,
                  resolvers res,
                  Ogre::SceneNode *);
};

} // namespace oo

#endif //OPENOBL_NPC__RESOLVER_HPP
