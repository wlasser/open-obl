#ifndef OPENOBLIVION_ENGINE_LIGHT_RESOLVER_HPP
#define OPENOBLIVION_ENGINE_LIGHT_RESOLVER_HPP

#include "engine/resolvers/resolvers.hpp"
#include "formid.hpp"
#include "records.hpp"
#include "ogrebullet/rigid_body.hpp"
#include <OgreColourValue.h>
#include <OgreLight.h>
#include <OgreSceneManager.h>
#include <string>

namespace engine {

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
  struct LightEntity {
    Ogre::Light *light{};
    Ogre::RigidBody *rigidBody{};
    Ogre::Entity *entity{};
  };

  std::unordered_map<BaseId, Entry> mMap{};

 public:
  using peek_t = Entry;
  using get_t = Entry;
  using make_t = LightEntity;
  using store_t = Entry;

  peek_t peek(BaseId baseId) const;
  get_t get(BaseId baseId) const;
  make_t make(BaseId baseId,
              gsl::not_null<Ogre::SceneManager *> mgr,
              std::optional<RefId> id) const;
  bool add(BaseId baseId, store_t entry);
};

} // namespace engine

#endif // OPENOBLIVION_ENGINE_LIGHT_RESOLVER_HPP
