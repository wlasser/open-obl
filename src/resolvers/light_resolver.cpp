#include "conversions.hpp"
#include "fs/path.hpp"
#include "game_settings.hpp"
#include "record/records.hpp"
#include "resolvers/helpers.hpp"
#include "resolvers/light_resolver.hpp"
#include <OgreColourValue.h>
#include <OgreLight.h>

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
    return {ecs::Light<>{nullptr}, ecs::RigidBody<>{nullptr},
            ecs::Mesh<>{nullptr}};
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

  {
    const auto &gs{oo::GameSettings::getSingleton()};
    const float s{oo::unitsPerMeter<float>};
    const float s1{gs.get("bLightAttenuation.fLinearRadiusMult", 1.0f)};
    const float s2{gs.get("bLightAttenuation.fQuadraticRadiusMult", 1.0f)};
    const float c0{gs.get("bLightAttenuation.fConstantValue", 1.0f)};
    const float c1{gs.get("bLightAttenuation.fLinearValue", 3.0f)};
    const float c2{gs.get("bLightAttenuation.fQuadraticValue", 16.0f)};

    const auto r{std::max(gsl::narrow_cast<float>(data.radius), 0.01f)};

    light->setAttenuation(r, c0,
                          s * c1 / (s1 * r),
                          s * s * c2 / (s2 * r * s2 * r));
    light->setPowerScale(baseRec->fadeValue ? baseRec->fadeValue->data : 1.0f);
  }

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

  return {ecs::Light<>{light}, ecs::RigidBody<>{rigidBody}, ecs::Mesh<>{mesh}};
}

} // namespace oo
