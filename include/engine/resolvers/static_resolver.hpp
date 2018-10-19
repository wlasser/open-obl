#ifndef OPENOBLIVION_ENGINE_STATIC_RESOLVER_HPP
#define OPENOBLIVION_ENGINE_STATIC_RESOLVER_HPP

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
  Ogre::RigidBody *rigidBody{nullptr};
  Ogre::Entity *entity{nullptr};
};

class StaticResolver {
 public:
  using get_t = RigidBodyEntity;
  using store_t = StaticEntry;

  RigidBodyEntity get(FormId baseID, Ogre::SceneManager *mgr) const;
  bool add(FormId baseID, store_t entry);

 private:
  std::unordered_map<FormId, store_t> statics{};
  friend class InitialProcessor;
};

} // namespace engine

#endif // OPENOBLIVION_ENGINE_STATIC_RESOLVER_HPP
