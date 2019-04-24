#include "fs/path.hpp"
#include "mesh/entity.hpp"
#include "nifloader/animation.hpp"
#include "record/records.hpp"
#include "resolvers/npc_resolver.hpp"
#include "resolvers/helpers.hpp"
#include "settings.hpp"
#include <OgreSkeletonManager.h>
#include <OgreSkeletonInstance.h>
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

namespace {

/// Get the base instance of the skeleton used by the `record::NPC_`.
Ogre::SkeletonPtr getSkeleton(const record::NPC_ &rec) {
  auto &skelMgr{Ogre::SkeletonManager::getSingleton()};
  if (!rec.skeletonFilename) return nullptr;
  const oo::Path filename{rec.skeletonFilename->data};
  const std::string path{(oo::Path{"meshes"} / filename).c_str()};
  return skelMgr.getByName(path, oo::RESOURCE_GROUP);
}

/// Return a path to the animation file for the given animation group.
/// \todo Animation groups are supposed to contain many different animations for
///       different situations. There are 'Forward' animations for when the
///       character is holding a one-handed weapon, a two-handed weapon, a
///       staff, and so on, for example. This function should choose the correct
///       animation out of a set, instead of just returning one.
std::string getAnimFromGroup(const std::string &animGroup) {
  const static std::map<std::string, std::string> animMap{
      {"AttackBow", "bowattack.kf"},
      {"BlockAttack", "blockattack.kf"},
      {"BlockHit", "blockhit.kf"},
      {"BlockIdle", "blockidle.kf"},
      {"CastSelf", "castself.kf"},
      {"CastSelfAlt", "castselfalt.kf"},
      {"CastTarget", "casttarget.kf"},
      {"CastTargetAlt", "casttargetalt.kf"},
      {"DodgeBack", "dodgeback.kf"},
      {"DodgeForward", "dodgeforward.kf"},
      {"DodgeLeft", "dodgeleft.kf"},
      {"DodgeRight", "dodgeright.kf"},
      {"Idle", "idle.kf"},
      {"JumpLand", "jumpland.kf"},
      {"JumpLoop", "jumploop.kf"},
      {"JumpStart", "jumpstart.kf"},
      {"TorchIdle", "torchidle.kf"},
      {"Backward", "walkbackward.kf"},
      {"Forward", "walkforward.kf"},
      {"Left", "walkleft.kf"},
      {"Right", "walkright.kf"},
      {"FastBackward", "walkfastbackward.kf"},
      {"FastForward", "walkfastforward.kf"},
      {"FastLeft", "walkfastleft.kf"},
      {"FastRight", "walkfastright.kf"},
      {"TurnLeft", "walkturnleft.kf"},
      {"TurnRight", "walkturnright.kf"},
  };

  return animMap.at(animGroup);
}

/// Given an animation filename and some context in which the animation is to
/// play, return the path of the animation file relative to `sMasterPath`.
oo::Path getAnimPath(const std::string &animName, bool firstPerson = false) {
  oo::Path basePath{firstPerson ? "meshes/characters/_1stPerson"
                                : "meshes/characters/_male"};
  return basePath / oo::Path{animName};
}

/// Return the animation with the given (full path) name owned by the
/// `skeleton`, creating the animation if the `skeleton` doesn't have an
/// animation with that name.
std::pair<Ogre::Animation *, bool>
createOrRetrieveAnimation(Ogre::Skeleton *skeleton,
                          const std::string &animPath) {
  if (skeleton->hasAnimation(animPath)) {
    return {skeleton->getAnimation(animPath), false};
  }
  return {oo::createAnimation(skeleton, animPath, oo::RESOURCE_GROUP), true};
}

/// Pick an idle animation for the `npc` and play it.
Ogre::AnimationState *pickIdle(oo::Entity *npc) {
  const auto filename{oo::getAnimFromGroup("Idle")};
  const auto path{oo::getAnimPath(filename, false)};
  auto[anim, created]{oo::createOrRetrieveAnimation(npc->getSkeleton(),
                                                    path.c_str())};
  if (created) npc->refreshAvailableAnimationState();

  auto *animState{npc->getAnimationState(anim->getName())};
  animState->setEnabled(true);
  animState->setTimePosition(0.0f);

  return animState;
}

} // namespace

template<> ReifyRecordTrait<record::REFR_NPC_>::type
reifyRecord(const record::REFR_NPC_ &refRec,
            gsl::not_null<Ogre::SceneManager *> scnMgr,
            gsl::not_null<btDiscreteDynamicsWorld *> world,
            ReifyRecordTrait<record::REFR_NPC_>::resolvers resolvers,
            Ogre::SceneNode *rootNode) {
  const auto &npc_Res{oo::getResolver<record::NPC_>(resolvers)};
  const auto &raceRes{oo::getResolver<record::RACE>(resolvers)};

  const auto baseRec{npc_Res.get(refRec.baseId.data)};
  if (!baseRec) return nullptr;

  const auto raceRec{raceRes.get(baseRec->race.data)};
  if (!raceRec) return nullptr;

  const auto &acbs{baseRec->baseConfig.data};
  using ACBSFlags = record::raw::ACBS::Flag;
  const bool female{acbs.flags & ACBSFlags::Female};

  auto baseSkel{oo::getSkeleton(*baseRec)};
  if (!baseSkel) return nullptr;
  baseSkel->load();

  auto *parent{(rootNode ? rootNode
                         : scnMgr->getRootSceneNode())->createChildSceneNode()};

  // Only one entity should have a skeleton constructed explicitly, the rest
  // should share. It doesn't matter which is entity is created first, we just
  // need one.
  oo::Entity *firstAdded{};
  // There needs to be a dedicated entity to attach the rigid bodies to though.
  oo::Entity *upperBody{};

  using BodyParts = record::raw::INDX_BODY;
  const auto &bodyData{female ? raceRec->femaleBodyData
                              : raceRec->maleBodyData};
  for (const auto &[typeRec, textureRec] : bodyData) {
    if (!textureRec) continue;
    const auto type{typeRec.data};
    const oo::Path texPath{oo::Path{"textures"} / oo::Path{textureRec->data}};
    const oo::Path meshPath = [female, type]() {
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
    const std::string meshName{meshPath.c_str()};
    if (meshName.empty()) continue;
    auto *node{oo::insertNif(meshName, oo::RESOURCE_GROUP,
                             scnMgr, world,
                             gsl::make_not_null(parent))};

    oo::Entity *entity = [&node]() -> oo::Entity * {
      for (auto obj : node->getAttachedObjects()) {
        if (auto *e{dynamic_cast<oo::Entity *>(obj)}) return e;
      }
      return nullptr;
    }();
    if (!entity) continue;

    if (!firstAdded) {
      firstAdded = entity;
      entity->setSkeleton(baseSkel);
      oo::pickIdle(entity);
    } else {
      entity->shareSkeleton(firstAdded);
    }
    if (type == BodyParts::UpperBody) upperBody = entity;
  }

  if (upperBody) {
    oo::attachRagdoll(baseSkel->getName(), oo::RESOURCE_GROUP, scnMgr, world,
                      gsl::make_not_null(upperBody));
  }

  return parent;
}

} // namespace oo
