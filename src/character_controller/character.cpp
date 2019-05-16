#include "character_controller/animation.hpp"
#include "character_controller/body.hpp"
#include "character_controller/character.hpp"
#include "resolvers/helpers.hpp"
#include "util/settings.hpp"
#include <OgreCamera.h>
#include <OgreSkeletonInstance.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

namespace oo {

Character::Character(const record::REFR_NPC_ &refRec,
                     gsl::not_null<Ogre::SceneManager *> scnMgr,
                     gsl::not_null<btDiscreteDynamicsWorld *> world,
                     Character::resolvers resolvers)
    : mMediator(oo::CharacterMediator(this)),
      mScnMgr(scnMgr),
      mPhysicsWorld(world),
      mState(oo::StandState{}),
      mMovementState(oo::WalkState{}) {
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

  mRoot = scnMgr->getRootSceneNode()->createChildSceneNode();

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
                             gsl::make_not_null(mRoot))};

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

  // TODO: Abstract away the camera somehow since it's only used for the player.
  const std::string cameraName{"__Camera" + oo::RefId{refRec.mFormId}.string()};
  mCamera = scnMgr->createCamera(cameraName);

  const auto h{0.95f * mHeight};
  const auto camVec{qvm::convert_to<Ogre::Vector3>(qvm::_0X0(h))};
  mCameraNode = mRoot->createChildSceneNode(camVec);
  mPitchNode = mCameraNode->createChildSceneNode();
  mPitchNode->attachObject(mCamera);

//  oo::attachRagdoll(getSkeleton()->getName(), oo::RESOURCE_GROUP, scnMgr, world,
//                    gsl::make_not_null(mBodyParts[0]));
  oo::pickIdle(this);
}

void Character::setBodyPart(oo::BodyParts part, oo::Entity *entity) noexcept {
  auto index{static_cast<std::underlying_type_t<oo::BodyParts>>(part)};
  mBodyParts[index] = entity;
}

void Character::enter(oo::StateVariant &state) {
  std::visit([this](auto &&s) { s.enter(this->mMediator); }, state);
}

void Character::enter(oo::MovementStateVariant &state) {
  std::visit([this](auto &&s) { s.enter(this->mMediator); }, state);
}

void Character::exit(oo::StateVariant &state) {
  std::visit([this](auto &&s) { s.exit(this->mMediator); }, state);
}

void Character::exit(oo::MovementStateVariant &state) {
  std::visit([this](auto &&s) { s.exit(this->mMediator); }, state);
}

void Character::changeState(oo::StateVariant newState) {
  exit(mState);
  mState = std::move(newState);
  enter(mState);
}

void Character::changeState(oo::MovementStateVariant newState) {
  exit(mMovementState);
  mMovementState = std::move(newState);
  enter(mMovementState);
}

void Character::updateCameraOrientation() noexcept {
  const Ogre::Quaternion q0(Ogre::Radian(0.0f), Ogre::Vector3::UNIT_X);
  mRoot->setOrientation(q0);
  mCameraNode->setOrientation(q0);
  mPitchNode->setOrientation(q0);

  mPitchNode->pitch(mPitch, Ogre::SceneNode::TS_LOCAL);
  mCameraNode->yaw(mYaw, Ogre::SceneNode::TS_LOCAL);
  mRoot->yaw(mRootYaw, Ogre::SceneNode::TS_LOCAL);
}

void Character::update(float elapsed) {
  if (auto newState{std::visit([this, elapsed](auto &&s) -> StateOpt {
      return s.update(mMediator, elapsed);
    }, mState)}; newState) {
    changeState(*newState);
  }

  if (auto newState{std::visit([this, elapsed](auto &&s) -> MovementStateOpt {
      return s.update(mMediator, elapsed);
    }, mMovementState)}; newState) {
    changeState(*newState);
  }
}

void Character::handleEvent(const oo::KeyVariant &event) {
//  if (auto newState{std::visit([this, &event](auto &&s) {
//      return std::visit([this, &s](auto &&e) -> StateOpt {
//        return s.handleEvent(mMediator, e);
//      }, event);
//    }, mState)}; newState) {
//    changeState(*newState);
//  }

//  if (auto newState{std::visit([this, &event](auto &&s) {
//      return std::visit([this, &s](auto &&e) -> MovementStateOpt {
//        return s.handleEvent(mMediator, e);
//      }, event);
//    }, mMovementState)}; newState) {
//    changeState(*newState);
//  }

  // TODO: Which method is better:
  //       - Nested visits for state then event
  //       - Multivariant visit
  //       - Single visit over event with a nested visit over each state
  if (auto newState{std::visit([this](auto &&s, auto &&e) -> StateOpt {
      return s.handleEvent(mMediator, e);
    }, mState, event)}; newState) {
    changeState(*newState);
  }

  if (auto newState{std::visit([this](auto &&s, auto &&e) -> MovementStateOpt {
      return s.handleEvent(mMediator, e);
    }, mMovementState, event)}; newState) {
    changeState(*newState);
  }
}

void Character::handleEvent(const oo::MouseVariant &event) {
  std::visit([this](auto &&s, auto &&e) { s.handleEvent(mMediator, e); },
             mState, event);

  std::visit([this](auto &&s, auto &&e) { s.handleEvent(mMediator, e); },
             mState, event);
}

void Character::setPosition(const Ogre::Vector3 &position) {
  mRoot->setPosition(position);
}

Ogre::Vector3 Character::getPosition() const {
  return mRoot->getPosition();
}

void Character::setOrientation(const Ogre::Quaternion &orientation) {
  mPitch = orientation.getPitch();
  mRootYaw = orientation.getYaw();
  mYaw = Ogre::Radian{0.0f};
  updateCameraOrientation();
}

Ogre::Camera *Character::getCamera() noexcept {
  return mCamera;
}

oo::Entity *Character::getBodyPart(oo::BodyParts part) noexcept {
  auto index{static_cast<std::underlying_type_t<oo::BodyParts>>(part)};
  return mBodyParts[index];
}

Ogre::SkeletonInstance *Character::getSkeleton() noexcept {
  return mBodyParts[0] ? mBodyParts[0]->getSkeleton() : nullptr;
}

int Character::getActorValue(oo::ActorValue actorValue) const noexcept {
  using UnderlyingType = std::underlying_type_t<oo::ActorValue>;
  return mActorValues[static_cast<UnderlyingType>(actorValue)];
}

} // namespace oo