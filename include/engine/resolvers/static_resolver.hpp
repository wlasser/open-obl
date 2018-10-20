#ifndef OPENOBLIVION_ENGINE_STATIC_RESOLVER_HPP
#define OPENOBLIVION_ENGINE_STATIC_RESOLVER_HPP

#include "engine/resolvers/resolvers.hpp"
#include "formid.hpp"
#include "records.hpp"
#include "ogrebullet/rigid_body.hpp"
#include <gsl/gsl>
#include <OgreEntity.h>
#include <OgreSceneManager.h>
#include <optional>
#include <string>
#include <unordered_map>

namespace engine {

using StaticResolver = Resolver<record::STAT>;

template<>
class Resolver<record::STAT> {
 private:
  struct Entry {
    std::string modelFilename{};
  };
  struct StaticEntity {
    Ogre::RigidBody *rigidBody{};
    Ogre::Entity *entity{};
  };

  std::unordered_map<BaseId, Entry> mMap{};

 public:
  using make_t = StaticEntity;
  using store_t = Entry;

  make_t make(BaseId baseId,
              gsl::not_null<Ogre::SceneManager *> mgr,
              std::optional<RefId> id) const;
  bool add(BaseId baseId, store_t entry);
};

} // namespace engine

#endif // OPENOBLIVION_ENGINE_STATIC_RESOLVER_HPP
