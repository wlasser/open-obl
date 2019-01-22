#include "conversions.hpp"
#include "fs/path.hpp"
#include "record/records.hpp"
#include "resolvers/helpers.hpp"
#include "resolvers/resolvers.hpp"
#include "resolvers/light_resolver.hpp"

namespace oo {

template<> oo::CiteRecordTrait<record::LIGH>::type
citeRecord(const record::LIGH &baseRec, tl::optional<RefId> refId) {
  record::REFR_LIGH::Raw rawRefRec{};
  rawRefRec.baseId = record::NAME(oo::BaseId{baseRec.mFormId});
  const record::REFR_LIGH refRec(rawRefRec,
                                 record::RecordFlag::None,
      // TODO: Get a new RefId properly
                                 static_cast<oo::FormId>(refId ? *refId
                                                               : oo::RefId{}),
                                 0);
  return refRec;
}

template<> oo::ReifyRecordTrait<record::REFR_LIGH>::type
reifyRecord(const record::REFR_LIGH &refRec,
            gsl::not_null<Ogre::SceneManager *> scnMgr,
            oo::ReifyRecordTrait<record::REFR_LIGH>::resolvers resolvers) {
  const auto &lighRes{oo::getResolver<record::LIGH>(resolvers)};
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

  Ogre::Entity *mesh{oo::loadMesh(*baseRec, scnMgr)};
  Ogre::RigidBody *rigidBody{oo::loadRigidBody(mesh, scnMgr)};

  if (rigidBody) {
    setRefId(gsl::make_not_null(rigidBody), oo::RefId{refRec.mFormId});
  }

  return {ecs::Light{light}, ecs::RigidBody{rigidBody}, ecs::Mesh{mesh}};
}

} // namespace oo
