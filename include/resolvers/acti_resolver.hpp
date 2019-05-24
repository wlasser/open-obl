#ifndef OPENOBL_ACTI_RESOLVER_HPP
#define OPENOBL_ACTI_RESOLVER_HPP

#include "resolvers/resolvers.hpp"

namespace oo {

using ActiResolver = Resolver<record::ACTI>;
using RefrActiResolver = Resolver<record::REFR_ACTI, RefId>;

template<> struct CiteRecordTrait<record::ACTI> {
  using type = record::REFR_ACTI;
};

template<> struct ReifyRecordTrait<record::REFR_ACTI> {
  using type = Ogre::SceneNode *;
  using resolvers = ResolverTuple<record::ACTI>;
};

template<> CiteRecordTrait<record::ACTI>::type
citeRecord(const record::ACTI &baseRec, tl::optional<RefId> refId);

template<> ReifyRecordTrait<record::REFR_ACTI>::type
reifyRecord(const record::REFR_ACTI &refRec,
            gsl::not_null<Ogre::SceneManager *> scnMgr,
            gsl::not_null<btDiscreteDynamicsWorld *> world,
            ReifyRecordTrait<record::REFR_ACTI>::resolvers resolvers,
            Ogre::SceneNode *rootNode);

} // namespace oo

#endif // OPENOBL_ACTI_RESOLVER_HPP
