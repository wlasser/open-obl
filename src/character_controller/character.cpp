#include "character_controller/animation.hpp"
#include "character_controller/body.hpp"
#include "character_controller/character.hpp"
#include "resolvers/helpers.hpp"
#include "settings.hpp"
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

  auto *parent{mController.getBodyNode()->createChildSceneNode(
      Ogre::Vector3{0.0f, -64 * oo::metersPerUnit<float>, 0.0f})};

  // Only one entity should have a skeleton constructed explicitly, the rest
  // should share. It doesn't matter which is entity is created first, we just
  // need one.
  oo::Entity *firstAdded{};
  // There needs to be a dedicated entity to attach the rigid bodies to though.
  oo::Entity *upperBody{};

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

    oo::Entity *entity = [&node]() -> oo::Entity * {
      for (auto obj : node->getAttachedObjects()) {
        if (auto *e{dynamic_cast<oo::Entity *>(obj)}) return e;
      }
      return nullptr;
    }();
    if (!entity) continue;

    oo::setSkinTextures(entity, oo::BaseId{raceRec->mFormId}, *textureRec);

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
}

oo::CharacterController &Character::getController() noexcept {
  return mController;
}

const oo::CharacterController &Character::getController() const noexcept {
  return mController;
}

} // namespace oo