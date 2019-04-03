#include "entity.hpp"
#include "fs/path.hpp"
#include "nifloader/animation.hpp"
#include "record/records.hpp"
#include "resolvers/npc_resolver.hpp"
#include "resolvers/helpers.hpp"
#include "settings.hpp"
#include <OgreSkeletonManager.h>
#include <map>

namespace oo {

template<> CiteRecordTrait<record::NPC_>::type
citeRecord(const record::NPC_ &baseRec, tl::optional<RefId> refId) {
  record::REFR_NPC_::Raw rawRefRec{};
  rawRefRec.baseId = record::NAME(BaseId{baseRec.mFormId});
  const record::REFR_NPC_ refRec(rawRefRec,
                                 record::RecordFlag::None,
      // TODO: Get a new RefId properly
                                 static_cast<FormId>(refId ? *refId : RefId{}),
                                 0);
  return refRec;
}

struct BodyData {
  oo::Entity *entity{};
  Ogre::RigidBody *rigidBody{};
  Ogre::TexturePtr texture{};
};

template<> ReifyRecordTrait<record::REFR_NPC_>::type
reifyRecord(const record::REFR_NPC_ &refRec,
            gsl::not_null<Ogre::SceneManager *> scnMgr,
            gsl::not_null<btDiscreteDynamicsWorld *> world,
            ReifyRecordTrait<record::REFR_NPC_>::resolvers resolvers,
            Ogre::SceneNode *rootNode) {
  const auto &npc_Res{oo::getResolver<record::NPC_>(resolvers)};
  const auto &raceRes{oo::getResolver<record::RACE>(resolvers)};

  auto baseRec{npc_Res.get(refRec.baseId.data)};
  if (!baseRec) return nullptr;

  auto raceRec{raceRes.get(baseRec->race.data)};
  if (!raceRec) return nullptr;

  const auto &acbs{baseRec->baseConfig.data};
  using ACBSFlags = record::raw::ACBS::Flag;
  const bool female{acbs.flags & ACBSFlags::Female};

  auto &texMgr{Ogre::TextureManager::getSingleton()};
  auto &skelMgr{Ogre::SkeletonManager::getSingleton()};

  if (!baseRec->skeletonFilename) return nullptr;

  const oo::Path rawSkelPath{baseRec->skeletonFilename->data};
  const std::string skelPath{(oo::Path{"meshes"} / rawSkelPath).c_str()};
  auto skelPtr{skelMgr.getByName(skelPath, oo::RESOURCE_GROUP)};
  skelPtr->load();

  Ogre::Animation *anim = [&]() {
    if (skelPtr->hasAnimation("Idle")) return skelPtr->getAnimation("Idle");
    return oo::createAnimation(skelPtr.get(),
                               "meshes/characters/_male/idle.kf",
                               oo::RESOURCE_GROUP);
  }();

  using BodyParts = record::raw::INDX_BODY;
  std::map<BodyParts, BodyData> bodyParts{};

  auto *parent{(rootNode ? rootNode : scnMgr->getRootSceneNode())
                   ->createChildSceneNode()};

  // Set in loop below.
  oo::Entity *upperBody{nullptr};

  const auto &bodyData{female ? raceRec->femaleBodyData
                              : raceRec->maleBodyData};
  for (const auto &[typeRec, textureRec] : bodyData) {
    if (!textureRec) continue;
    const auto type{typeRec.data};
    const oo::Path texPath{oo::Path{"textures"} / oo::Path{textureRec->data}};
    const oo::Path meshPath = [&]() {
      switch (type) {
        case BodyParts::UpperBody:
          return oo::Path{female ? "meshes/characters/_male/femaleupperbody.nif"
                                 : "meshes/characters/_male/upperbody.nif"};
        case BodyParts::LowerBody:
          return oo::Path{female ? "meshes/characters/_male/femalelowerbody.nif"
                                 : "meshes/characters/_male/lowerbody.nif"};
        case BodyParts::Hand:
          return oo::Path{female ? "meshes/characters/_male/femalehand.nif"
                                 : "meshes/characters/_male/hand.nif"};
        case BodyParts::Foot:
          return oo::Path{female ? "meshes/characters/_male/femalefoot.nif"
                                 : "meshes/characters/_male/foot.nif"};
        default: return oo::Path{""};
      }
    }();
    auto &part{bodyParts[type]};
    const std::string meshName{meshPath.c_str()};
    if (meshName.empty()) continue;
    auto *node{oo::insertNif(meshName,
                             oo::RESOURCE_GROUP,
                             scnMgr,
                             world,
                             gsl::make_not_null(parent))};

    oo::Entity *entity = [&]() -> oo::Entity * {
      for (auto obj : node->getAttachedObjects()) {
        if (auto *e{dynamic_cast<oo::Entity *>(obj)}) return e;
      }
      return nullptr;
    }();
    if (!entity) continue;

    if (upperBody) {
      entity->shareSkeleton(upperBody);
    } else {
      entity->setSkeleton(skelPtr);
    }

    if (type == BodyParts::UpperBody) upperBody = entity;

    auto *animState{entity->getAnimationState(anim->getName())};
    animState->setEnabled(true);
    animState->setTimePosition(0.0f);

//    auto texture{texMgr.load(texPath.c_str(), oo::RESOURCE_GROUP)};
  }

  if (upperBody) {
    oo::attachRagdoll(skelPath, oo::RESOURCE_GROUP, scnMgr, world,
                      gsl::make_not_null(upperBody));
  }

  return parent;
}

} // namespace oo
