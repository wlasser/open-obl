#ifndef OPENOBLIVION_ENGINE_DOOR_RESOLVER_HPP
#define OPENOBLIVION_ENGINE_DOOR_RESOLVER_HPP

#include "engine/resolvers/resolvers.hpp"
#include "records.hpp"
#include "ogrebullet/rigid_body.hpp"
#include <OgreEntity.h>
#include <string>

namespace engine {

using DoorResolver = Resolver<record::DOOR>;

template<>
class Resolver<record::DOOR> {
 private:
  struct Entry {
    std::string modelFilename{};
    bool oblivionGate{false};
    bool automatic{false};
    bool hidden{false};
    bool minimalUses{false};
    // TODO: Door sounds
  };
  struct DoorEntity {
    Ogre::RigidBody *rigidBody{};
    Ogre::Entity *entity{};
  };

  std::unordered_map<BaseId, Entry> mMap{};

 public:
  using make_t = DoorEntity;
  using store_t = Entry;

  make_t make(BaseId baseId,
              gsl::not_null<Ogre::SceneManager *> mgr,
              std::optional<RefId> id) const;
  bool add(BaseId baseId, store_t entry);
};

} // namespace engine

#endif // OPENOBLIVION_ENGINE_DOOR_RESOLVER_HPP
