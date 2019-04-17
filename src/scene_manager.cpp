#include "scene_manager.hpp"
#include "settings.hpp"
#include <OgreRoot.h>
#include <spdlog/spdlog.h>

namespace oo {

DeferredSceneManager::DeferredSceneManager(const Ogre::String &name)
    : Ogre::SceneManager(name) {}

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
  SceneManager::destroyMovableObject(name, typeName);
  if (typeName == Ogre::LightFactory::FACTORY_TYPE_NAME) {
    auto pred = [&name](const auto &info) {
      return info.light->getName() == name;
    };
    auto it{std::find_if(mLights.begin(), mLights.end(), pred)};
    if (it == mLights.end()) return;

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

DeferredSceneManager::LightInfo::LightInfo(Ogre::Light *light,
                                           std::unique_ptr<oo::DeferredLight> geometry)
    : light(light), geometry(std::move(geometry)) {}

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