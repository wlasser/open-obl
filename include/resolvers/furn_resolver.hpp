#ifndef OPENOBLIVION_FURN_RESOLVER_HPP
#define OPENOBLIVION_FURN_RESOLVER_HPP

#include "resolvers/resolvers.hpp"

namespace oo {

using FurnResolver = Resolver<record::FURN>;
using RefrFurnResolver = Resolver<record::REFR_FURN, RefId>;

template<> struct CiteRecordTrait<record::FURN> {
  using type = record::REFR_FURN;
};

template<> struct ReifyRecordTrait<record::REFR_FURN> {
  using type = Ogre::SceneNode *;
  using resolvers = ResolverTuple<record::FURN>;
};

template<> CiteRecordTrait<record::FURN>::type
citeRecord(const record::FURN &baseRec, tl::optional<RefId> refId);

template<> ReifyRecordTrait<record::REFR_FURN>::type
reifyRecord(const record::REFR_FURN &refRec,
            gsl::not_null<Ogre::SceneManager *> scnMgr,
            gsl::not_null<btDiscreteDynamicsWorld *> world,
            ReifyRecordTrait<record::REFR_FURN>::resolvers resolvers,
            Ogre::SceneNode *rootNode);

} // namespace oo

#endif // OPENOBLIVION_FURN_RESOLVER_HPP
