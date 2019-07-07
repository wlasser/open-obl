#include "bullet/collision.hpp"
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

  const float capsuleHeight{mHeight - 2.0f * CAPSULE_RADIUS};
  mCapsuleShape = std::make_unique<btCapsuleShape>(CAPSULE_RADIUS,
                                                   capsuleHeight);
  mCapsule = std::make_unique<btCollisionObject>();
  mCapsule->setCollisionShape(mCapsuleShape.get());
  constexpr int flags{btCollisionObject::CollisionFlags::CF_KINEMATIC_OBJECT};
  mCapsule->setCollisionFlags(flags);

  constexpr auto collisionLayer{bullet::CollisionLayer::OL_BIPED};
  // See p1481, discussed at Kona but not approved.
  //C++23: constexpr auto[group, mask]{bullet::getCollisionFilter(collisionLayer)};
  constexpr auto gm{bullet::getCollisionFilter(collisionLayer)};
  world->addCollisionObject(mCapsule.get(), std::get<0>(gm), std::get<1>(gm));

//  oo::attachRagdoll(getSkeleton()->getName(), oo::RESOURCE_GROUP, scnMgr, world,
//                    gsl::make_not_null(mBodyParts[0]));
  oo::pickIdle(this);
}

Character::~Character() {
  if (mPhysicsWorld && mCapsule) {
    mPhysicsWorld->removeCollisionObject(mCapsule.get());
  }

  if (mScnMgr) {
    // Destroying a node detaches all its attached objects and removes their
    // children, though it is not clear whether those children are deleted.
    if (mPitchNode) mScnMgr->destroySceneNode(mPitchNode);
    if (mCameraNode) mScnMgr->destroySceneNode(mCameraNode);
    if (mRoot) mScnMgr->destroySceneNode(mRoot);
    if (mCamera) mScnMgr->destroyCamera(mCamera);
  }
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

void Character::updateTwist() noexcept {
  // Camera and root yaw may not be aligned currently, but they should be when
  // the player is moving. Smooth the camera yaw to zero while keeping the
  // absolute camera orientation and movement direction the same.
  // twistMultiplier should be in the range [0, 1), with higher values giving
  // slower body rotation times.
  constexpr float twistMul{0.75f};
  mRootYaw += mYaw * (1.0f - twistMul);
  mYaw *= twistMul;
}

void Character::orientCamera() noexcept {
  const Ogre::Quaternion q0(Ogre::Radian(0.0f), Ogre::Vector3::UNIT_X);
  mRoot->setOrientation(q0);
  mCameraNode->setOrientation(q0);
  mPitchNode->setOrientation(q0);

  mPitchNode->pitch(mPitch, Ogre::SceneNode::TS_LOCAL);
  mCameraNode->yaw(mYaw, Ogre::SceneNode::TS_LOCAL);
  mRoot->yaw(mRootYaw, Ogre::SceneNode::TS_LOCAL);
}

void Character::updateCamera() noexcept {
  updateTwist();
  orientCamera();
}

void Character::updateCapsule() noexcept {
  const auto orientation{mRoot->getOrientation()};
  auto position{mRoot->getPosition()};
  qvm::Y(position) += 0.5f * mHeight;
  mCapsule->setWorldTransform(btTransform(
      qvm::convert_to<btQuaternion>(orientation),
      qvm::convert_to<btVector3>(position)));
}

Character::RaycastResult Character::raycast() const noexcept {
  const Ogre::Vector3 offset{Ogre::Vector3::UNIT_Y * mHeight * 0.5f};
  const auto p0{qvm::convert_to<btVector3>(mRoot->getPosition() + offset)};
  const auto p1{p0 - qvm::_0X0(MAX_RAYCAST_DISTANCE)};
  RaycastResult result(mCapsule.get(), p0, p1);
  mPhysicsWorld->rayTest(p0, p1, result);
  return result;
}

Ogre::Vector4 Character::getSurfaceNormal() const noexcept {
  constexpr float twoPiBy3{2.09439510f};
  const auto rootPos{qvm::convert_to<btVector3>(mRoot->getPosition())};
  auto rootDir = [this]() -> btVector3 {
    const btVector3 v = qvm::X0Z(mVelocity);
    float norm = qvm::mag(v);
    return norm < 0.1f ? btVector3(1.0f, 0.0f, 0.0f) : v / norm;
  }();
  const float offset{mHeight * 0.5f};
  const btMatrix3x3 rotMat = qvm::roty_mat<3>(-twoPiBy3);

  std::array<btVector3, 3u> dirs{
      rootDir,
      rotMat * rootDir,
      rotMat * rotMat * rootDir,
  };

  std::array<btVector3, 3u> starts{
      rootPos + 0.3f * dirs[0] + qvm::_0X0(offset),
      rootPos + 0.3f * dirs[1] + qvm::_0X0(offset),
      rootPos + 0.3f * dirs[2] + qvm::_0X0(offset)
  };

  // Can't use array, even though we know the size, because RaycastResult is not
  // default constructible.
  std::vector<RaycastResult> results;
  btVector3 midpoint{0.0f, 0.0f, 0.0f};

  for (std::size_t i = 0; i < 3u; ++i) {
    const auto end{starts[i] - qvm::_0X0(MAX_RAYCAST_DISTANCE)};
    auto &r{results.emplace_back(mCapsule.get(), starts[i], end)};
    mPhysicsWorld->rayTest(starts[i], end, r);

    // Need at least three points to compute a normal, give up if we're clipping
    // out of the world.
    if (!r.m_hasHit) return {0.0f, 1.0f, 0.0f, MAX_RAYCAST_DISTANCE};

    midpoint += r.m_hitPointWorld / 3.0f;
  }

  const auto r1{results[1].m_hitPointWorld - results[0].m_hitPointWorld};
  const auto r2{results[2].m_hitPointWorld - results[0].m_hitPointWorld};

  return Ogre::Vector4(qvm::convert_to<Ogre::Vector3>(
      qvm::normalized(qvm::cross(r1, r2))),
                       qvm::mag(midpoint - rootPos - qvm::_0X0(offset)));
}

Ogre::Matrix3 Character::getSurfaceFrame() const noexcept {
  auto normalDist{getSurfaceNormal()};
  const float dist{qvm::W(normalDist)};
  const Ogre::Vector3 normal = qvm::XYZ(normalDist);

  Ogre::Vector3 tangent{-mRoot->getLocalAxes().GetColumn(2)};
  tangent -= qvm::dot(tangent, normal) * normal;
  qvm::normalize(tangent);

  const Ogre::Vector3 binormal = qvm::cross(tangent, normal);
  Ogre::Matrix3 frame;
  frame.FromAxes(binormal, normal, tangent);

  return frame;
}

std::optional<float> Character::getSurfaceDist() const noexcept {
  const auto r{raycast()};
  if (r.m_hasHit) {
    return qvm::mag(r.m_rayFromWorld - r.m_hitPointWorld) - mHeight * 0.5f;
  } else {
    return {};
  }
}

Ogre::Matrix3 Character::getDefaultFrame() const noexcept {
  return mRoot->getLocalAxes();
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

float Character::getMoveSpeed() const noexcept {
  const float speed{oo::baseSpeed(getActorValue(ActorValue::Speed))};
  const float heightMul{mRaceHeight};
  const float weightMul{oo::encumbranceModifier(mWornWeight, mHasWeaponOut)};
  return speed * heightMul * weightMul
      * (mSpeedModifier ? mSpeedModifier(mHasWeaponOut, mIsRunning) : 1.0f)
      * oo::metersPerUnit<float>;
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
  orientCamera();
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