#include "game_settings.hpp"
#include "math/conversions.hpp"
#include "record/records.hpp"
#include "resolvers/helpers.hpp"
#include "resolvers/ligh_resolver.hpp"
#include <boost/algorithm/string/predicate.hpp>
#include <OgreColourValue.h>
#include <OgreLight.h>
#include <OgreSceneNode.h>

namespace oo {

auto
CiteRecordImpl<record::LIGH>::operator()(const record::LIGH &baseRec,
                                         tl::optional<RefId> refId) -> type {
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

namespace {

template<class F> Ogre::SceneNode *findChild(Ogre::SceneNode *parent, F &&f) {
  if (f(parent)) return parent;
  for (Ogre::Node *childNode : parent->getChildren()) {
    auto *childSceneNode{static_cast<Ogre::SceneNode *>(childNode)};
    if (auto *foundNode{oo::findChild(childSceneNode,
                                      std::forward<F>(f))})
      return foundNode;
  }
  return nullptr;
}

} // namespace oo

auto ReifyRecordImpl<record::REFR_LIGH>::operator()(
    const record::REFR_LIGH &refRec,
    Ogre::SceneManager *scnMgr,
    btDiscreteDynamicsWorld *world,
    resolvers res,
    Ogre::SceneNode *rootNode) -> type {
  const auto &lighRes{oo::getResolver<record::LIGH>(res)};
  auto baseRec{lighRes.get(refRec.baseId.data)};
  if (!baseRec) return nullptr;

  const auto &data{baseRec->data.data};

  auto *const light{scnMgr->createLight()};
  const Ogre::ColourValue lightColor = [&data]() -> Ogre::ColourValue {
    Ogre::ColourValue col{};
    col.setAsABGR(data.color);
    return col;
  }();
  light->setDiffuseColour(lightColor);
  light->setSpecularColour(lightColor);

  {
    const auto &gs{oo::GameSettings::getSingleton()};
    const float s{oo::metersPerUnit<float>};
    const float s1{gs.get("bLightAttenuation.fLinearRadiusMult", 1.0f)};
    const float s2{gs.get("bLightAttenuation.fQuadraticRadiusMult", 1.0f)};
    const float c0{gs.get("bLightAttenuation.fConstantValue", 1.0f)};
    const float c1{gs.get("bLightAttenuation.fLinearValue", 3.0f)};
    const float c2{gs.get("bLightAttenuation.fQuadraticValue", 16.0f)};

    const auto r{std::max(gsl::narrow_cast<float>(data.radius) * s, 0.01f)};

    light->setAttenuation(r, c0,
                          c1 / (s1 * r),
                          c2 / (s2 * r * s2 * r));
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

  auto *baseNode{oo::insertNif(*baseRec, oo::RefId{refRec.mFormId},
                               gsl::make_not_null(scnMgr),
                               gsl::make_not_null(world),
                               rootNode)};
  if (!baseNode) {
    baseNode = rootNode ? rootNode->createChildSceneNode()
                        : scnMgr->getRootSceneNode()->createChildSceneNode();
  }

  auto *lightNode = [&]() {
    auto *attachNode{oo::findChild(baseNode, [](Ogre::SceneNode *child) {
      return boost::iends_with(child->getName(), "AttachLight");
    })};
    return attachNode ? attachNode : baseNode;
  }();

  lightNode->attachObject(light);

  return baseNode;
}

} // namespace oo
