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
  Ogre::Entity *entity{};
  Ogre::RigidBody *rigidBody{};
  Ogre::TexturePtr texture{};
};

template<> ReifyRecordTrait<record::REFR_NPC_>::type
reifyRecord(const record::REFR_NPC_ &refRec,
            gsl::not_null<Ogre::SceneManager *> scnMgr,
            ReifyRecordTrait<record::REFR_NPC_>::resolvers resolvers) {
  const auto &npc_Res{oo::getResolver<record::NPC_>(resolvers)};
  const auto &raceRes{oo::getResolver<record::RACE>(resolvers)};

  // Object to return if loading fails
  const ReifyRecordTrait<record::REFR_NPC_>::type empty{
      ecs::RigidBody<0>{nullptr},
      ecs::RigidBody<1>{nullptr}, ecs::Mesh<1>{nullptr},
      ecs::RigidBody<2>{nullptr}, ecs::Mesh<2>{nullptr},
      ecs::RigidBody<3>{nullptr}, ecs::Mesh<3>{nullptr},
      ecs::RigidBody<4>{nullptr}, ecs::Mesh<4>{nullptr},
  };

  auto baseRec{npc_Res.get(refRec.baseId.data)};
  if (!baseRec) return empty;

  auto raceRec{raceRes.get(baseRec->race.data)};
  if (!raceRec) return empty;

  const auto &acbs{baseRec->baseConfig.data};
  using ACBSFlags = record::raw::ACBS::Flag;
  const bool female{acbs.flags & ACBSFlags::Female};

  auto &texMgr{Ogre::TextureManager::getSingleton()};
  auto &meshMgr{Ogre::MeshManager::getSingleton()};
  auto &skelMgr{Ogre::SkeletonManager::getSingleton()};

  if (!baseRec->skeletonFilename) return empty;

  const oo::Path rawSkelPath{baseRec->skeletonFilename->data};
  const std::string skelPath{(oo::Path{"meshes"} / rawSkelPath).c_str()};
  auto skelPtr{skelMgr.getByName(skelPath, oo::RESOURCE_GROUP)};
  auto *skelBox{oo::loadRigidBody(skelPath, oo::RESOURCE_GROUP, scnMgr)};
  skelPtr->load();

  auto *anim{oo::createAnimation(skelPtr.get(),
                                 "meshes/characters/_male/idle.kf",
                                 oo::RESOURCE_GROUP)};

  using BodyParts = record::raw::INDX_BODY;
  std::map<BodyParts, BodyData> bodyParts{};

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
    part.entity = scnMgr->createEntity(meshName);
    part.entity->getMesh()->setSkeletonName(skelPtr->getName());
    part.entity->_initialise(true);

    auto *animState{part.entity->getAnimationState(anim->getName())};
    animState->setEnabled(true);
    animState->setTimePosition(0.0f);

    part.rigidBody = oo::loadRigidBody(meshName, oo::RESOURCE_GROUP, scnMgr);
    part.texture = texMgr.load(texPath.c_str(), oo::RESOURCE_GROUP);
  }

  return {ecs::RigidBody<0>{skelBox},
          ecs::RigidBody<1>{bodyParts[BodyParts::UpperBody].rigidBody},
          ecs::Mesh<1>{bodyParts[BodyParts::UpperBody].entity},
          ecs::RigidBody<2>{bodyParts[BodyParts::LowerBody].rigidBody},
          ecs::Mesh<2>{bodyParts[BodyParts::LowerBody].entity},
          ecs::RigidBody<3>{bodyParts[BodyParts::Hand].rigidBody},
          ecs::Mesh<3>{bodyParts[BodyParts::Hand].entity},
          ecs::RigidBody<4>{bodyParts[BodyParts::Foot].rigidBody},
          ecs::Mesh<4>{bodyParts[BodyParts::Foot].entity},
  };
}

} // namespace oo
