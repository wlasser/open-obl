#ifndef OPENOBLIVION_STATIC_RESOLVER_HPP
#define OPENOBLIVION_STATIC_RESOLVER_HPP

#include "record/formid.hpp"
#include "record/records_fwd.hpp"
#include "record/reference_records.hpp"
#include "resolvers/ecs.hpp"
#include "resolvers/resolvers.hpp"
#include "ogrebullet/rigid_body.hpp"
#include <gsl/gsl>
#include <OgreEntity.h>
#include <OgreSceneManager.h>
#include <optional>
#include <string>

namespace oo {

using StatResolver = Resolver<record::STAT>;
using RefrStatResolver = Resolver<record::REFR_STAT, RefId>;

template<>
struct CiteRecordTrait<record::STAT> {
  using type = record::REFR_STAT;
};

template<>
struct ReifyRecordTrait<record::REFR_STAT> {
  using type = ecs::Entity<ecs::RigidBody, ecs::Mesh>;
  using resolvers = std::tuple<const Resolver<record::STAT> &>;
};

template<>
CiteRecordTrait<record::STAT>::type
citeRecord(const record::STAT &baseRec, tl::optional<RefId> refId);

template<>
ReifyRecordTrait<record::REFR_STAT>::type
reifyRecord(const record::REFR_STAT &refRec,
            gsl::not_null<Ogre::SceneManager *> scnMgr,
            ReifyRecordTrait<record::REFR_STAT>::resolvers resolvers);

} // namespace oo

#endif // OPENOBLIVION_STATIC_RESOLVER_HPP
