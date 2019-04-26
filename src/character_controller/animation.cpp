#include "character_controller/animation.hpp"
#include "nifloader/animation.hpp"
#include "record/records.hpp"
#include "settings.hpp"
#include <OgreSkeletonManager.h>
#include <OgreSkeletonInstance.h>

namespace oo {

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

oo::Path getAnimPath(const std::string &animName, bool firstPerson) {
  oo::Path basePath{firstPerson ? "meshes/characters/_1stPerson"
                                : "meshes/characters/_male"};
  return basePath / oo::Path{animName};
}

std::pair<Ogre::Animation *, bool>
createOrRetrieveAnimation(Ogre::Skeleton *skeleton,
                          const std::string &animPath) {
  if (skeleton->hasAnimation(animPath)) {
    return {skeleton->getAnimation(animPath), false};
  }
  return {oo::createAnimation(skeleton, animPath, oo::RESOURCE_GROUP), true};
}

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

/// Get the base instance of the skeleton used by the `record::NPC_`.
Ogre::SkeletonPtr getSkeleton(const record::NPC_ &rec) {
  auto &skelMgr{Ogre::SkeletonManager::getSingleton()};
  if (!rec.skeletonFilename) return nullptr;
  const oo::Path filename{rec.skeletonFilename->data};
  const std::string path{(oo::Path{"meshes"} / filename).c_str()};
  return skelMgr.getByName(path, oo::RESOURCE_GROUP);
}

} // namespace oo