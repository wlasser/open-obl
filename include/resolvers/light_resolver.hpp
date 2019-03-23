#ifndef OPENOBLIVION_LIGHT_RESOLVER_HPP
#define OPENOBLIVION_LIGHT_RESOLVER_HPP

#include "resolvers/resolvers.hpp"
#include "ogrebullet/rigid_body.hpp"

namespace oo {

using LighResolver = Resolver<record::LIGH>;
using RefrLighResolver = Resolver<record::REFR_LIGH, RefId>;

template<>
struct CiteRecordTrait<record::LIGH> {
  using type = record::REFR_LIGH;
};

template<>
struct ReifyRecordTrait<record::REFR_LIGH> {
  using type = Ogre::SceneNode *;
  using resolvers = ResolverTuple<record::LIGH>;
};

template<>
CiteRecordTrait<record::LIGH>::type
citeRecord(const record::LIGH &baseRec, tl::optional<RefId> refId);

template<>
ReifyRecordTrait<record::REFR_LIGH>::type
reifyRecord(const record::REFR_LIGH &refRec,
            gsl::not_null<Ogre::SceneManager *> scnMgr,
            gsl::not_null<btDiscreteDynamicsWorld *> world,
            ReifyRecordTrait<record::REFR_LIGH>::resolvers resolvers,
            Ogre::SceneNode *rootNode);

} // namespace oo

#endif // OPENOBLIVION_LIGHT_RESOLVER_HPP
