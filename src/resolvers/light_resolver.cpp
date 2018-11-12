#include "conversions.hpp"
#include "fs/path.hpp"
#include "resolvers/resolvers.hpp"
#include "resolvers/light_resolver.hpp"

template<>
ReifyRecordTrait<record::LIGH>::type
reifyRecord(const record::LIGH &rec,
            gsl::not_null<Ogre::SceneManager *> scnMgr,
            tl::optional<RefId> refId) {
  const auto &data{rec.data.data};

  auto *const light{scnMgr->createLight()};
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

  Ogre::Entity *mesh{loadMesh(rec, scnMgr)};
  Ogre::RigidBody *rigidBody{loadRigidBody(mesh, scnMgr)};

  if (rigidBody) {
    // TODO: Get a new RefId properly
    setRefId(gsl::make_not_null(rigidBody), refId ? *refId : RefId{});
  }

  return {ecs::Light{light}, ecs::RigidBody{rigidBody}, ecs::Mesh{mesh}};
}
