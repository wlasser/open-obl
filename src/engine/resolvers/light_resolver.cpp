#include "engine/conversions.hpp"
#include "engine/resolvers/resolvers.hpp"
#include "engine/resolvers/light_resolver.hpp"

engine::LightMesh engine::LightResolver::get(BaseId baseId,
                                             Ogre::SceneManager *mgr) const {
  const auto entry{lights.find(baseId)};
  if (entry == lights.end()) return {};

  const auto &rec{entry->second};

  auto *const light{mgr->createLight()};

  light->setDiffuseColour(rec.color);
  light->setSpecularColour(rec.color);
  // TODO: These could do with more tuning
  const auto radius{rec.radius * conversions::metersPerUnit<float>};
  light->setAttenuation(radius,
                        1.0f,
                        3.0f / radius,
                        5.0f / (radius * radius));
  light->setPowerScale(rec.fadeValue);

  const auto spotLightFlag
      {LightEntry::Flag::SpotLight | LightEntry::Flag::SpotShadow};

  if (rec.flags & spotLightFlag) {
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

  return {light, rigidBody, mesh};
}

bool engine::LightResolver::add(BaseId baseId, store_t entry) {
  return lights.try_emplace(baseId, entry).second;
}
