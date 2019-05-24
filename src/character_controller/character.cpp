#include "character_controller/animation.hpp"
#include "character_controller/body.hpp"
#include "character_controller/character.hpp"
#include "resolvers/helpers.hpp"
#include "util/settings.hpp"
#include <OgreSkeletonInstance.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

namespace oo {

Character::Character(const record::REFR_NPC_ &refRec,
                     gsl::not_null<Ogre::SceneManager *> scnMgr,
                     gsl::not_null<btDiscreteDynamicsWorld *> world,
                     Character::resolvers resolvers)
    : mController(scnMgr, world) {
  const auto &npc_Res{oo::getResolver<record::NPC_>(resolvers)};
  const auto &raceRes{oo::getResolver<record::RACE>(resolvers)};

  const auto baseRec{npc_Res.get(refRec.baseId.data)};
  if (!baseRec) {
    spdlog::get(oo::LOG)->error("NPC_ record {} does not exist",
                                refRec.baseId.data);
    throw std::runtime_error("NPC_ record does not exist");
  }

  const auto raceRec{raceRes.get(baseRec->race.data)};
  if (!raceRec) {
    spdlog::get(oo::LOG)->error("RACE record {} does not exist",
                                refRec.baseId.data);
    throw std::runtime_error("RACE record does not exist");
  }

  const auto &acbs{baseRec->baseConfig.data};
  using ACBSFlags = record::raw::ACBS::Flag;
  const bool female{acbs.flags & ACBSFlags::Female};

  auto baseSkel{oo::getSkeleton(*baseRec)};
  if (!baseSkel) {
    spdlog::get(oo::LOG)->error("NPC_ record {} has no skeleton",
                                oo::BaseId{baseRec->mFormId});
    throw std::runtime_error("NPC_ record has no skeleton");
  }
  baseSkel->load();

  auto *parent{mController.getRootNode()->createChildSceneNode()};

  // Only one entity should have a skeleton constructed explicitly, the rest
  // should share. It doesn't matter which is entity is created first, since
  // they all share with each other, we just need one.
  oo::Entity *firstAdded{};

  const auto &bodyData{female ? raceRec->femaleBodyData
                              : raceRec->maleBodyData};
  for (const auto &[typeRec, textureRec] : bodyData) {
    if (!textureRec) continue;
    const auto type{typeRec.data};
    const std::string meshName{oo::getBodyPartPath(type, female).c_str()};
    if (meshName.empty()) continue;
    auto *node{oo::insertNif(meshName, oo::RESOURCE_GROUP,
                             scnMgr, world,
                             gsl::make_not_null(parent))};

    auto &entities{node->getAttachedObjects()};
    auto it{std::find_if(entities.rbegin(), entities.rend(), [](auto *ent) {
      return dynamic_cast<oo::Entity *>(ent) != nullptr;
    })};
    auto *entity{it != entities.rend() ? static_cast<oo::Entity *>(*it)
                                       : nullptr};
    if (!entity) continue;

    setBodyPart(type, entity);
    oo::setSkinTextures(entity, oo::BaseId{raceRec->mFormId}, *textureRec);

    if (!firstAdded) {
      firstAdded = entity;
      entity->setSkeleton(baseSkel);
    } else {
      entity->shareSkeleton(firstAdded);
    }
  }

  oo::attachRagdoll(getSkeleton()->getName(), oo::RESOURCE_GROUP, scnMgr, world,
                    gsl::make_not_null(mBodyParts[0]));
  oo::pickIdle(this);
}

void Character::setBodyPart(oo::BodyParts part, oo::Entity *entity) noexcept {
  auto index{static_cast<std::underlying_type_t<oo::BodyParts>>(part)};
  mBodyParts[index] = entity;
}

oo::CharacterController &Character::getController() noexcept {
  return mController;
}

const oo::CharacterController &Character::getController() const noexcept {
  return mController;
}

Ogre::SkeletonInstance *Character::getSkeleton() noexcept {
  return mBodyParts[0] ? mBodyParts[0]->getSkeleton() : nullptr;
}

oo::Entity *Character::getBodyPart(oo::BodyParts part) noexcept {
  auto index{static_cast<std::underlying_type_t<oo::BodyParts>>(part)};
  return mBodyParts[index];
}

} // namespace oo