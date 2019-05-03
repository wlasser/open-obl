#ifndef OPENOBLIVION_FLOR_RESOLVER_HPP
#define OPENOBLIVION_FLOR_RESOLVER_HPP

#include "resolvers/resolvers.hpp"

namespace oo {

using FlorResolver = Resolver<record::FLOR>;
using RefrFlorResolver = Resolver<record::REFR_FLOR, RefId>;

template<> struct CiteRecordTrait<record::FLOR> {
  using type = record::REFR_FLOR;
};

template<> struct ReifyRecordTrait<record::REFR_FLOR> {
  using type = Ogre::SceneNode *;
  using resolvers = ResolverTuple<record::FLOR>;
};

template<> CiteRecordTrait<record::FLOR>::type
citeRecord(const record::FLOR &baseRec, tl::optional<RefId> refId);

template<> ReifyRecordTrait<record::REFR_FLOR>::type
reifyRecord(const record::REFR_FLOR &refRec,
            gsl::not_null<Ogre::SceneManager *> scnMgr,
            gsl::not_null<btDiscreteDynamicsWorld *> world,
            ReifyRecordTrait<record::REFR_FLOR>::resolvers resolvers,
            Ogre::SceneNode *rootNode);

} // namespace oo

#endif // OPENOBLIVION_FLOR_RESOLVER_HPP
