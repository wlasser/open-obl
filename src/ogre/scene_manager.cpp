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

Ogre::MovableObject *
DeferredSceneManager::createMovableObject(const Ogre::String &name,
                                          const Ogre::String &typeName,
                                          const Ogre::NameValuePairList *params) {
  auto *obj{Ogre::SceneManager::createMovableObject(name, typeName, params)};
  if (typeName == Ogre::LightFactory::FACTORY_TYPE_NAME) {
    auto *light{static_cast<Ogre::Light *>(obj)};
    mLights.emplace_back(light, std::make_unique<oo::DeferredLight>(light));
  }
  return obj;
}

void DeferredSceneManager::destroyMovableObject(const Ogre::String &name,
                                                const Ogre::String &typeName) {
  if (typeName == Ogre::LightFactory::FACTORY_TYPE_NAME) {
    auto pred = [&name](const auto &info) {
      return info.light->getName() == name;
    };
    auto it{std::find_if(mLights.begin(), mLights.end(), pred)};
    if (it == mLights.end()) return;

    SceneManager::destroyMovableObject(name, typeName);
    mLights.erase(it);
  }
}

void DeferredSceneManager::destroyAllMovableObjectsByType(const Ogre::String &typeName) {
  Ogre::SceneManager::destroyAllMovableObjectsByType(typeName);
  if (typeName == Ogre::LightFactory::FACTORY_TYPE_NAME) mLights.clear();
}

void DeferredSceneManager::destroyAllMovableObjects() {
  Ogre::SceneManager::destroyAllMovableObjects();
  mLights.clear();
}

std::vector<oo::DeferredLight *> DeferredSceneManager::getLights() const {
  std::vector<oo::DeferredLight *> lights(mLights.size());
  std::transform(mLights.begin(), mLights.end(), lights.begin(),
                 [](const auto &info) { return info.geometry.get(); });
  return lights;
}

oo::DeferredFogListener *DeferredSceneManager::getFogListener() noexcept {
  return &mFogListener;
}

DeferredSceneManager::LightInfo::LightInfo(Ogre::Light *pLight,
                                           std::unique_ptr<oo::DeferredLight> pGeometry)
    : light(pLight), geometry(std::move(pGeometry)) {}

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