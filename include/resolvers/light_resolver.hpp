#ifndef OPENOBLIVION_LIGHT_RESOLVER_HPP
#define OPENOBLIVION_LIGHT_RESOLVER_HPP

#include "resolvers/ecs.hpp"
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
  using type = ecs::Entity<ecs::Light<>, ecs::RigidBody<>, ecs::Mesh<>>;
  using resolvers = ResolverTuple<record::LIGH>;
};

template<>
CiteRecordTrait<record::LIGH>::type
citeRecord(const record::LIGH &baseRec, tl::optional<RefId> refId);

template<>
ReifyRecordTrait<record::REFR_LIGH>::type
reifyRecord(const record::REFR_LIGH &refRec,
            gsl::not_null<Ogre::SceneManager *> scnMgr,
            ReifyRecordTrait<record::REFR_LIGH>::resolvers resolvers);

} // namespace oo

#endif // OPENOBLIVION_LIGHT_RESOLVER_HPP
