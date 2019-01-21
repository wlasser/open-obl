#ifndef OPENOBLIVION_ACTI_RESOLVER_HPP
#define OPENOBLIVION_ACTI_RESOLVER_HPP

#include "record/records_fwd.hpp"
#include "record/reference_records.hpp"
#include "resolvers/ecs.hpp"
#include "resolvers/resolvers.hpp"
#include "ogrebullet/rigid_body.hpp"
#include <OgreEntity.h>

namespace oo {

using ActiResolver = Resolver<record::ACTI>;
using RefrActiResolver = Resolver<record::REFR_ACTI, RefId>;

template<> struct CiteRecordTrait<record::ACTI> {
  using type = record::REFR_ACTI;
};

template<> struct ReifyRecordTrait<record::REFR_ACTI> {
  using type = ecs::Entity<ecs::RigidBody, ecs::Mesh>;
  using resolvers = std::tuple<const Resolver<record::ACTI> &>;
};

template<> CiteRecordTrait<record::ACTI>::type
citeRecord(const record::ACTI &baseRec, tl::optional<RefId> refId);

template<> ReifyRecordTrait<record::REFR_ACTI>::type
reifyRecord(const record::REFR_ACTI &refRec,
            gsl::not_null<Ogre::SceneManager *> scnMgr,
            ReifyRecordTrait<record::REFR_ACTI>::resolvers resolvers);

} // namespace oo

#endif // OPENOBLIVION_ACTI_RESOLVER_HPP
