#ifndef OPENOBLIVION_ENGINE_STATIC_MANAGER_HPP
#define OPENOBLIVION_ENGINE_STATIC_MANAGER_HPP

#include "ogrebullet/rigid_body.hpp"
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
  Ogre::RigidBody *rigidBody{};
  Ogre::Entity *entity{};
};

class StaticManager {
 private:
  std::unordered_map<FormID, StaticEntry> statics{};
  friend class InitialProcessor;

 public:
  RigidBodyEntity get(FormID baseID, Ogre::SceneManager *mgr) const {
    const auto entry = statics.find(baseID);
    if (entry != statics.end()) {
      const auto &name = entry->second.modelFilename;
      // TODO: Name entity according to ref id
      auto *entity = mgr->createEntity(name);
      const auto &group = entity->getMesh()->getGroup();
      std::map<std::string, std::string> params = {
          {"collisionObject", name},
          {"resourceGroup", group}
      };
      // Yes, we are using an exception for control flow. It is necessary, see
      // RigidBodyFactory::createInstanceImpl.
      // TODO: Replace with a mgr->createRigidBody on a derived SceneManager
      try {
        auto *rigidBody = dynamic_cast<Ogre::RigidBody *>(
            mgr->createMovableObject("RigidBody", &params));
        return {rigidBody, entity};
      } catch (const Ogre::PartialCollisionObjectException &e) {
        return {nullptr, entity};
      }
    } else return {nullptr, nullptr};
  }
};

} // namespace engine

#endif // OPENOBLIVION_ENGINE_STATIC_MANAGER_HPP
