#ifndef OPENOBLIVION_ENGINE_STATIC_MANAGER_HPP
#define OPENOBLIVION_ENGINE_STATIC_MANAGER_HPP

#include "formid.hpp"
#include <OgreEntity.h>
#include <OgreSceneManager.h>
#include <string>
#include <unordered_map>

namespace engine {

struct StaticEntry {
  std::string modelFilename{};
};

class StaticManager {
 private:
  std::unordered_map<FormID, StaticEntry> statics{};
  friend class InitialProcessor;

 public:
  Ogre::Entity *get(FormID baseID, Ogre::SceneManager *mgr) {
    auto entry = statics.find(baseID);
    if (entry != statics.end())
      return mgr->createEntity(entry->second.modelFilename);
    else return nullptr;
  }
};

} // namespace engine

#endif // OPENOBLIVION_ENGINE_STATIC_MANAGER_HPP
