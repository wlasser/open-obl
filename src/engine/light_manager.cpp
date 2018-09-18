#include "engine/light_manager.hpp"

Ogre::Light *engine::LightManager::get(FormID baseID, Ogre::SceneManager *mgr) {
  auto entry = lights.find(baseID);
  if (entry != lights.end()) {
    const auto &rec = entry->second;

    auto *light = mgr->createLight();

    light->setDiffuseColour(rec.color);
    light->setSpecularColour(rec.color);
    // TODO: These could do with more tuning
    light->setAttenuation(rec.radius,
                          1.0f,
                          0.25f / rec.radius,
                          10.0f / (rec.radius * rec.radius));
    light->setPowerScale(rec.fadeValue);

    auto spotLightFlag =
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

    return light;
  } else return nullptr;
}