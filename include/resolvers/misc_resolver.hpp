#ifndef OPENOBL_MISC_RESOLVER_HPP
#define OPENOBL_MISC_RESOLVER_HPP

#include "resolvers/resolvers.hpp"

namespace oo {

using MiscResolver = Resolver<record::MISC>;
using RefrMiscResolver = Resolver<record::REFR_MISC, RefId>;

template<>
struct CiteRecordTrait<record::MISC> {
  using type = record::REFR_MISC;
};

template<>
struct ReifyRecordTrait<record::REFR_MISC> {
  using type = Ogre::SceneNode *;
  using resolvers = ResolverTuple<record::MISC>;
};

template<> CiteRecordTrait<record::MISC>::type
citeRecord(const record::MISC &baseRec, tl::optional<RefId> refId);

template<> ReifyRecordTrait<record::REFR_MISC>::type
reifyRecord(const record::REFR_MISC &refRec,
            gsl::not_null<Ogre::SceneManager *> scnMgr,
            gsl::not_null<btDiscreteDynamicsWorld *> world,
            ReifyRecordTrait<record::REFR_MISC>::resolvers resolvers,
            Ogre::SceneNode *rootNode);

} // namespace oo

#endif // OPENOBL_MISC_RESOLVER_HPP
