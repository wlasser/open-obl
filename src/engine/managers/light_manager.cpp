#include <engine/conversions.hpp>
#include "engine/managers/light_manager.hpp"

engine::LightMesh engine::LightManager::get(FormID baseID,
                                            Ogre::SceneManager *mgr) const {
  const auto entry = lights.find(baseID);
  if (entry != lights.end()) {
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

    auto *const mesh = [=]() -> Ogre::Entity * {
      if (rec.modelFilename.empty()) return nullptr;
      else return mgr->createEntity(rec.modelFilename);
    }();

    if (!mesh) return {light, nullptr, mesh};

    auto *const rigidBody = [=]() -> Ogre::RigidBody * {
      const auto &group{mesh->getMesh()->getGroup()};
      std::map<std::string, std::string> params = {
          {"collisionObject", rec.modelFilename},
          {"resourceGroup", group}
      };
      try {
        return dynamic_cast<Ogre::RigidBody *>(
            mgr->createMovableObject("RigidBody", &params));
      } catch (const Ogre::PartialCollisionObjectException &e) {
        return nullptr;
      }
    }();

    return {light, rigidBody, mesh};
  } else return {};
}