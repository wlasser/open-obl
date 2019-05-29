#include "ogre/scene_manager.hpp"
#include <OgreCamera.h>
#include <OgreRoot.h>

namespace oo {

//===----------------------------------------------------------------------===//
// Deferred Lighting Base Scene Manager
//===----------------------------------------------------------------------===//
DeferredSceneManager::DeferredSceneManager(const Ogre::String &name)
    : Ogre::SceneManager(name), mFogListener(this) {}

const Ogre::String &DeferredSceneManager::getTypeName() const {
  static Ogre::String typeName{DeferredSceneManagerFactory::FACTORY_TYPE_NAME};
  return typeName;
}

oo::DeferredLight *DeferredSceneManager::createLight(const Ogre::String &name) {
  return static_cast<oo::DeferredLight *>(
      createMovableObject(name, DeferredLightFactory::FACTORY_TYPE_NAME));
}

oo::DeferredLight *DeferredSceneManager::getLight(const Ogre::String &name) const {
  return static_cast<oo::DeferredLight *>(
      getMovableObject(name, DeferredLightFactory::FACTORY_TYPE_NAME));
}

bool DeferredSceneManager::hasLight(const Ogre::String &name) const {
  return hasMovableObject(name, DeferredLightFactory::FACTORY_TYPE_NAME);
}

void DeferredSceneManager::destroyLight(const Ogre::String &name) {
  return destroyMovableObject(name, DeferredLightFactory::FACTORY_TYPE_NAME);
}

void DeferredSceneManager::destroyAllLights() {
  return destroyAllMovableObjectsByType(DeferredLightFactory::FACTORY_TYPE_NAME);
}

void
DeferredSceneManager::findLightsAffectingFrustum(const Ogre::Camera *camera) {
  static const std::string factoryName{DeferredLightFactory::FACTORY_TYPE_NAME};
  const auto &lights{getMovableObjectCollection(factoryName)->map};

  std::vector<LightInfo> infos;
  infos.reserve(lights.size());

  for (const auto &[_, obj] : lights) {
    auto *light{static_cast<Ogre::Light *>(obj)};
    light->_setCameraRelative(mCameraRelativeRendering ? mCameraInProgress
                                                       : nullptr);
    if (!light->isVisible()) continue;

    LightInfo info;
    info.light = light;
    info.type = light->getType();
    info.lightMask = light->getLightMask();

    if (light->getType() == Ogre::Light::LightTypes::LT_DIRECTIONAL) {
      info.position = Ogre::Vector3::ZERO;
      info.range = 0.0f;
      infos.push_back(info);
    } else {
      // TODO: Support spotlights.
      info.position = light->getDerivedPosition(mCameraRelativeRendering);
      info.range = light->getAttenuationRange();
      if (camera->isVisible(Ogre::Sphere(info.position, info.range))) {
        infos.push_back(info);
      }
    }
  }

  if (mCachedLightInfos != infos) {
    mLightsAffectingFrustum.resize(infos.size());
    std::transform(infos.begin(), infos.end(), mLightsAffectingFrustum.begin(),
                   [](const auto &info) { return info.light; });

    // Sort lights for texture shadows because the closest n are used to
    // generate textures.
    if (isShadowTechniqueTextureBased()) {
      for (auto *light : mLightsAffectingFrustum) {
        light->_calcTempSquareDist(camera->getDerivedPosition());
      }

      // Listeners who do not wish to sort return false and do nothing.
      auto rbegin{mListeners.rbegin()}, rend{mListeners.rend()};
      const bool sorted{rend != std::find_if(rbegin, rend, [&](auto *l) {
        return l->sortLightsAffectingFrustum(mLightsAffectingFrustum);
      })};
      if (!sorted) {
        std::stable_sort(mLightsAffectingFrustum.begin(),
                         mLightsAffectingFrustum.end(),
                         lightsForShadowTextureLess{});
      }
    }

    mCachedLightInfos.swap(infos);
    _notifyLightsDirty();
  }
}

oo::DeferredFogListener *DeferredSceneManager::getFogListener() noexcept {
  return &mFogListener;
}

//===----------------------------------------------------------------------===//
// Scene Manager Factories
//===----------------------------------------------------------------------===//
gsl::owner<Ogre::SceneManager *>
DeferredSceneManagerFactory::createInstance(const Ogre::String &instanceName) {
  return OGRE_NEW oo::DeferredSceneManager(instanceName);
}

void DeferredSceneManagerFactory::destroyInstance(gsl::owner<Ogre::SceneManager *> instance) {
  OGRE_DELETE instance;
}

void DeferredSceneManagerFactory::initMetaData() const {
  mMetaData.typeName = FACTORY_TYPE_NAME;
  mMetaData.worldGeometrySupported = false;
}

} // namespace oo