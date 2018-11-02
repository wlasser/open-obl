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
  struct ItemEntry {
    std::string name{};
    std::string iconFilename{};
    int32_t value{};
    float weight{};
    int32_t time{};
  };
  struct Entry {
    using Flag = record::raw::DATA_LIGH::Flag;
    std::string modelFilename{};
    BaseId sound{};
    BaseId script{};
    float fadeValue{1.0f};
    float radius{300.0f};
    float falloffExponent{1.0f};
    float fov{90.0f};
    Ogre::ColourValue color{};
    Flag flags{Flag::make(Flag::None)};
    std::optional<ItemEntry> item{};
  };

  std::unordered_map<BaseId, Entry> mMap{};

 public:
  using peek_t = Entry;
  using get_t = Entry;
  using make_t = ecs::Entity<ecs::Light, ecs::RigidBody, ecs::Mesh>;
  using store_t = Entry;

  peek_t peek(BaseId baseId) const;
  get_t get(BaseId baseId) const;
  make_t make(BaseId baseId,
              gsl::not_null<Ogre::SceneManager *> mgr,
              std::optional<RefId> id) const;
  bool add(BaseId baseId, store_t entry);
  bool contains(BaseId baseId) const noexcept;
};

#endif // OPENOBLIVION_LIGHT_RESOLVER_HPP
