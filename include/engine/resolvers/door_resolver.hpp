#ifndef OPENOBLIVION_ENGINE_DOOR_RESOLVER_HPP
#define OPENOBLIVION_ENGINE_DOOR_RESOLVER_HPP

#include "records.hpp"
#include "ogrebullet/rigid_body.hpp"
#include <OgreEntity.h>
#include <string>

namespace engine {

struct DoorEntry {
  std::string modelFilename{};
  bool oblivionGate{false};
  bool automatics{false};
  bool hidden{false};
  bool minimalUse{false};
  // TODO: Door sounds
};

struct DoorEntity {
  Ogre::RigidBody *rigidBody{nullptr};
  Ogre::Entity *entity{nullptr};
};

class DoorResolver {
 public:
  using get_t = DoorEntity;
  using store_t = DoorEntry;

  get_t get(BaseId baseId, Ogre::SceneManager *mgr) const;
  bool add(BaseId baseId, store_t entry);

 private:
  std::unordered_map<BaseId, store_t> doors{};
};

} // namespace engine

#endif // OPENOBLIVION_ENGINE_DOOR_RESOLVER_HPP
