#ifndef OPENOBLIVION_DOOR_RESOLVER_HPP
#define OPENOBLIVION_DOOR_RESOLVER_HPP

#include "records.hpp"
#include "resolvers/resolvers.hpp"
#include "ogrebullet/rigid_body.hpp"
#include <OgreEntity.h>
#include <string>

using DoorResolver = Resolver<record::DOOR>;

template<>
struct ReifyRecordTrait<record::DOOR> {
  using type = ecs::Entity<ecs::RigidBody, ecs::Mesh>;
};

template<>
ReifyRecordTrait<record::DOOR>::type
reifyRecord(const record::DOOR &rec,
            gsl::not_null<Ogre::SceneManager *> scnMgr,
            tl::optional<RefId> refId);

#endif // OPENOBLIVION_DOOR_RESOLVER_HPP
