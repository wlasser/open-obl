#include <engine/conversions.hpp>
#include "engine/managers/light_manager.hpp"

engine::LightMesh engine::LightManager::get(FormID baseID,
                                            Ogre::SceneManager *mgr) const {
  const auto entry = lights.find(baseID);
  if (entry != lights.end()) {
    const auto &rec = entry->second;

    auto *light = mgr->createLight();

    light->setDiffuseColour(rec.color);
    light->setSpecularColour(rec.color);
    // TODO: These could do with more tuning
    const auto radius = rec.radius * conversions::metersPerUnit<float>;
    light->setAttenuation(radius,
                          1.0f,
                          3.0f / radius,
                          5.0f / (radius * radius));
    light->setPowerScale(rec.fadeValue);

    Ogre::Entity *mesh = nullptr;
    if (!rec.modelFilename.empty()) mesh = mgr->createEntity(rec.modelFilename);

    const auto spotLightFlag =
        LightEntry::Flag::SpotLight | LightEntry::Flag::SpotShadow;

    if ((rec.flags & spotLightFlag) != LightEntry::Flag::None) {
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

    return {light, mesh};
  } else return {};
}