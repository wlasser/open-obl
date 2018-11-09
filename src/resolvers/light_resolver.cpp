#include "conversions.hpp"
#include "fs/path.hpp"
#include "resolvers/resolvers.hpp"
#include "resolvers/light_resolver.hpp"

auto Resolver<record::LIGH>::peek(BaseId baseId) const -> peek_t {
  const auto entry{mMap.find(baseId)};
  if (entry == mMap.end()) return {};
  return entry->second;
}

auto Resolver<record::LIGH>::get(BaseId baseId) const -> get_t {
  return peek(baseId);
}

auto Resolver<record::LIGH>::make(BaseId baseId,
                                  gsl::not_null<Ogre::SceneManager *> mgr,
                                  std::optional<RefId> id) const -> make_t {
  const auto entry{mMap.find(baseId)};
  if (entry == mMap.end()) return {};
  const auto &rec{entry->second};
  const auto &data{rec.data.data};

  auto *const light{mgr->createLight()};
  const Ogre::ColourValue lightColor = [&data]() -> Ogre::ColourValue {
    Ogre::ColourValue col{};
    col.setAsABGR(data.color.v);
    return col;
  }();
  light->setDiffuseColour(lightColor);
  light->setSpecularColour(lightColor);

  // TODO: These could do with more tuning
  const auto radius{std::max(gsl::narrow_cast<float>(data.radius)
                                 * conversions::metersPerUnit<float>, 0.01f)};
  light->setAttenuation(radius, 1.0f, 3.0f / radius, 5.0f / (radius * radius));

  light->setPowerScale(rec.fadeValue ? rec.fadeValue->data : 1.0f);

  using Flag = record::raw::DATA_LIGH::Flag;
  const auto spotlightFlag{Flag::SpotLight | Flag::SpotShadow};
  if (data.flags & spotlightFlag) {
    // Spotlights
    light->setType(Ogre::Light::LightTypes::LT_SPOTLIGHT);
    light->setSpotlightRange(Ogre::Radian(0.0f),
                             Ogre::Degree(data.fov),
                             data.falloffExponent);
    light->setSpotlightNearClipDistance(0.0f);
  } else {
    // Point lights
    light->setType(Ogre::Light::LightTypes::LT_POINT);
  }

  Ogre::Entity *mesh = [&rec, mgr]() -> Ogre::Entity * {
    if (rec.modelFilename) {
      fs::Path rawPath{rec.modelFilename->data};
      std::string meshName{(fs::Path{"meshes"} / rawPath).c_str()};
      return loadMesh(meshName, mgr);
    } else return nullptr;
  }();

  Ogre::RigidBody *rigidBody{loadRigidBody(mesh, mgr)};

  if (rigidBody) {
    // TODO: Get a new RefId properly
    const RefId refId{id ? *id : RefId{}};
    setRefId(gsl::make_not_null(rigidBody), refId);
  }

  return {ecs::Light{light}, ecs::RigidBody{rigidBody}, ecs::Mesh{mesh}};
}

bool Resolver<record::LIGH>::add(BaseId baseId, store_t entry) {
  return mMap.insert_or_assign(baseId, entry).second;
}

bool Resolver<record::LIGH>::contains(BaseId baseId) const noexcept {
  return mMap.find(baseId) != mMap.end();
}
