#ifndef OPENOBLIVION_LIGHT_RESOLVER_HPP
#define OPENOBLIVION_LIGHT_RESOLVER_HPP

#include "formid.hpp"
#include "records.hpp"
#include "resolvers/resolvers.hpp"
#include "ogrebullet/rigid_body.hpp"
#include <OgreColourValue.h>
#include <OgreLight.h>
#include <OgreSceneManager.h>
#include <string>

using LightResolver = Resolver<record::LIGH>;

template<>
class Resolver<record::LIGH> {
 private:
  std::unordered_map<BaseId, record::LIGH> mMap{};

 public:
  using peek_t = record::LIGH;
  using get_t = record::LIGH;
  using make_t = ecs::Entity<ecs::Light, ecs::RigidBody, ecs::Mesh>;
  using store_t = record::LIGH;

  peek_t peek(BaseId baseId) const;
  get_t get(BaseId baseId) const;
  make_t make(BaseId baseId,
              gsl::not_null<Ogre::SceneManager *> mgr,
              std::optional<RefId> id) const;
  bool add(BaseId baseId, store_t entry);
  bool contains(BaseId baseId) const noexcept;
};

#endif // OPENOBLIVION_LIGHT_RESOLVER_HPP
