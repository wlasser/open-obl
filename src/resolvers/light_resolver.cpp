#include "conversions.hpp"
#include "resolvers/resolvers.hpp"
#include "resolvers/light_resolver.hpp"

auto Resolver<record::LIGH>::peek(BaseId baseId) const -> peek_t {
  const auto entry{mMap.find(baseId)};
  if (entry == mMap.end()) return {};
  return entry->second;
}

auto Resolver<record::LIGH>::get(BaseId baseId) const -> get_t {
  return peek(baseId);
}

auto Resolver<record::LIGH>::make(BaseId baseId,
                                  gsl::not_null<Ogre::SceneManager *> mgr,
                                  std::optional<RefId> id) const -> make_t {
  const auto entry{mMap.find(baseId)};
  if (entry == mMap.end()) return {};
  const auto &rec{entry->second};

  auto *const light{mgr->createLight()};
  light->setDiffuseColour(rec.color);
  light->setSpecularColour(rec.color);
  // TODO: These could do with more tuning
  const auto radius
      {std::max(rec.radius * conversions::metersPerUnit<float>, 0.01f)};
  light->setAttenuation(radius,
                        1.0f,
                        3.0f / radius,
                        5.0f / (radius * radius));
  light->setPowerScale(rec.fadeValue);

  const auto spotlightFlag{Entry::Flag::SpotLight | Entry::Flag::SpotShadow};
  if (rec.flags & spotlightFlag) {
    // Spotlights
    light->setType(Ogre::Light::LightTypes::LT_SPOTLIGHT);
    light->setSpotlightRange(Ogre::Radian(0.0f),
                             Ogre::Degree(rec.fov),
                             rec.falloffExponent);
    light->setSpotlightNearClipDistance(0.0f);
  } else {
    // Point lights
    light->setType(Ogre::Light::LightTypes::LT_POINT);
  }

  auto *const mesh{loadMesh(rec, mgr)};
  auto *const rigidBody{loadRigidBody(mesh, mgr)};

  if (rigidBody) {
    // TODO: Get a new RefId properly
    const RefId refId{id ? *id : RefId{}};
    setRefId(gsl::make_not_null(rigidBody), refId);
  }

  return {ecs::Light{light}, ecs::RigidBody{rigidBody}, ecs::Mesh{mesh}};
}

bool Resolver<record::LIGH>::add(BaseId baseId, store_t entry) {
  return mMap.insert_or_assign(baseId, entry).second;
}

bool Resolver<record::LIGH>::contains(BaseId baseId) const noexcept {
  return mMap.find(baseId) != mMap.end();
}
