#ifndef OPENOBLIVION_STATIC_RESOLVER_HPP
#define OPENOBLIVION_STATIC_RESOLVER_HPP

#include "formid.hpp"
#include "records.hpp"
#include "resolvers/resolvers.hpp"
#include "ogrebullet/rigid_body.hpp"
#include <gsl/gsl>
#include <OgreEntity.h>
#include <OgreSceneManager.h>
#include <optional>
#include <string>
#include <unordered_map>

using StaticResolver = Resolver<record::STAT>;

template<>
struct ReifyRecordTrait<record::STAT> {
  using type = ecs::Entity<ecs::RigidBody, ecs::Mesh>;
};

template<>
ReifyRecordTrait<record::STAT>::type
reifyRecord(const record::STAT &rec,
            gsl::not_null<Ogre::SceneManager *> scnMgr,
            tl::optional<RefId> refId);

#endif // OPENOBLIVION_STATIC_RESOLVER_HPP
