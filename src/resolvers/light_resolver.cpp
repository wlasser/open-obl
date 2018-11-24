#include "conversions.hpp"
#include "fs/path.hpp"
#include "resolvers/resolvers.hpp"
#include "resolvers/light_resolver.hpp"

template<>
CiteRecordTrait<record::LIGH>::type
citeRecord(const record::LIGH &baseRec, tl::optional<RefId> refId) {
  record::REFR_LIGH::Raw rawRefRec{};
  rawRefRec.baseId = record::NAME(BaseId{baseRec.mFormId});
  const record::REFR_LIGH refRec(rawRefRec,
                                 record::RecordFlag::None,
      // TODO: Get a new RefId properly
                                 static_cast<FormId>(refId ? *refId : RefId{}),
                                 0);
  return refRec;
}

template<>
ReifyRecordTrait<record::REFR_LIGH>::type
reifyRecord(const record::REFR_LIGH &refRec,
            gsl::not_null<Ogre::SceneManager *> scnMgr,
            ReifyRecordTrait<record::REFR_LIGH>::resolvers resolvers) {
  const auto &lighRes{std::get<const Resolver<record::LIGH> &>(resolvers)};
  auto baseRec{lighRes.get(refRec.baseId.data)};
  if (!baseRec) {
    return {ecs::Light{nullptr}, ecs::RigidBody{nullptr}, ecs::Mesh{nullptr}};
  }

  const auto &data{baseRec->data.data};

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
                                 * oo::metersPerUnit<float>, 0.01f)};
  light->setAttenuation(radius, 1.0f, 3.0f / radius, 5.0f / (radius * radius));

  light->setPowerScale(baseRec->fadeValue ? baseRec->fadeValue->data : 1.0f);

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

  Ogre::Entity *mesh{loadMesh(*baseRec, scnMgr)};
  Ogre::RigidBody *rigidBody{loadRigidBody(mesh, scnMgr)};

  if (rigidBody) {
    setRefId(gsl::make_not_null(rigidBody), RefId{refRec.mFormId});
  }

  return {ecs::Light{light}, ecs::RigidBody{rigidBody}, ecs::Mesh{mesh}};
}
