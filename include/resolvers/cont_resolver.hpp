#ifndef OPENOBL_CONT_RESOLVER_HPP
#define OPENOBL_CONT_RESOLVER_HPP

#include "resolvers/resolvers.hpp"

namespace oo {

using ContResolver = Resolver<record::CONT>;
using RefrContResolver = Resolver<record::REFR_CONT, RefId>;

template<> struct CiteRecordTrait<record::CONT> {
  using type = record::REFR_CONT;
};

template<> struct ReifyRecordTrait<record::REFR_CONT> {
  using type = Ogre::SceneNode *;
  using resolvers = ResolverTuple<record::CONT>;
};

template<> CiteRecordTrait<record::CONT>::type
citeRecord(const record::CONT &baseRec, tl::optional<RefId> refId);

template<> ReifyRecordTrait<record::REFR_CONT>::type
reifyRecord(const record::REFR_CONT &refRec,
            gsl::not_null<Ogre::SceneManager *> scnMgr,
            gsl::not_null<btDiscreteDynamicsWorld *> world,
            ReifyRecordTrait<record::REFR_CONT>::resolvers resolvers,
            Ogre::SceneNode *rootNode);

} // namespace oo

#endif // OPENOBL_CONT_RESOLVER_HPP
