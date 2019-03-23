#ifndef OPENOBLIVION_STATIC_RESOLVER_HPP
#define OPENOBLIVION_STATIC_RESOLVER_HPP

#include "resolvers/resolvers.hpp"

namespace oo {

using StatResolver = Resolver<record::STAT>;
using RefrStatResolver = Resolver<record::REFR_STAT, RefId>;

template<>
struct CiteRecordTrait<record::STAT> {
  using type = record::REFR_STAT;
};

template<>
struct ReifyRecordTrait<record::REFR_STAT> {
  using type = Ogre::SceneNode *;
  using resolvers = ResolverTuple<record::STAT>;
};

template<>
CiteRecordTrait<record::STAT>::type
citeRecord(const record::STAT &baseRec, tl::optional<RefId> refId);

template<>
ReifyRecordTrait<record::REFR_STAT>::type
reifyRecord(const record::REFR_STAT &refRec,
            gsl::not_null<Ogre::SceneManager *> scnMgr,
            gsl::not_null<btDiscreteDynamicsWorld *> world,
            ReifyRecordTrait<record::REFR_STAT>::resolvers resolvers,
            Ogre::SceneNode *rootNode);

} // namespace oo

#endif // OPENOBLIVION_STATIC_RESOLVER_HPP
