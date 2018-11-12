#ifndef OPENOBLIVION_LIGHT_RESOLVER_HPP
#define OPENOBLIVION_LIGHT_RESOLVER_HPP

#include "formid.hpp"
#include "records.hpp"
#include "resolvers/resolvers.hpp"
#include "ogrebullet/rigid_body.hpp"
#include <OgreColourValue.h>
#include <OgreLight.h>
#include <OgreSceneManager.h>
#include <string>

using LightResolver = Resolver<record::LIGH>;

template<>
struct ReifyRecordTrait<record::LIGH> {
  using type = ecs::Entity<ecs::Light, ecs::RigidBody, ecs::Mesh>;
};

template<>
ReifyRecordTrait<record::LIGH>::type
reifyRecord(const record::LIGH &rec,
            gsl::not_null<Ogre::SceneManager *> scnMgr,
            tl::optional<RefId> refId);

#endif // OPENOBLIVION_LIGHT_RESOLVER_HPP
