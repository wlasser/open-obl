#ifndef OPENOBLIVION_ENGINE_STATIC_MANAGER_HPP
#define OPENOBLIVION_ENGINE_STATIC_MANAGER_HPP

#include "engine/ogre/rigid_body_manager.hpp"
#include "formid.hpp"
#include <OgreEntity.h>
#include <OgreSceneManager.h>
#include <string>
#include <unordered_map>

namespace engine {

struct StaticEntry {
  std::string modelFilename{};
};

struct RigidBodyEntity {
  std::shared_ptr<Ogre::RigidBody> rigidBody{};
  Ogre::Entity *entity{};
};

class StaticManager {
 private:
  std::unordered_map<FormID, StaticEntry> statics{};
  friend class InitialProcessor;

 public:
  RigidBodyEntity get(FormID baseID, Ogre::SceneManager *mgr) {
    auto entry = statics.find(baseID);
    if (entry != statics.end()) {
      const auto &name = entry->second.modelFilename;
      auto *entity = mgr->createEntity(name);
      const auto &group = entity->getMesh()->getGroup();

      auto &rigidBodyMgr = Ogre::RigidBodyManager::getSingleton();

      auto retrieveResult = rigidBodyMgr.createOrRetrieve(name, group);
      auto rigidBody =
          std::dynamic_pointer_cast<Ogre::RigidBody>(retrieveResult.first);
      if (rigidBody != nullptr) rigidBody->load(false);

      return {rigidBody, entity};
    } else return {nullptr, nullptr};
  }
};

} // namespace engine

#endif // OPENOBLIVION_ENGINE_STATIC_MANAGER_HPP
