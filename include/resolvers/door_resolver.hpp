#ifndef OPENOBLIVION_DOOR_RESOLVER_HPP
#define OPENOBLIVION_DOOR_RESOLVER_HPP

#include "records.hpp"
#include "resolvers/resolvers.hpp"
#include "ogrebullet/rigid_body.hpp"
#include <OgreEntity.h>
#include <string>

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

  std::unordered_map<BaseId, Entry> mMap{};

 public:
  using make_t = ecs::Entity<ecs::RigidBody, ecs::Mesh>;
  using store_t = Entry;

  make_t make(BaseId baseId,
              gsl::not_null<Ogre::SceneManager *> mgr,
              std::optional<RefId> id) const;
  bool add(BaseId baseId, store_t entry);
  bool contains(BaseId baseId) const noexcept;
};

#endif // OPENOBLIVION_DOOR_RESOLVER_HPP
