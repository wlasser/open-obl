#include "bullet/collision.hpp"
#include "character_controller/character_controller_impl.hpp"
#include "character_controller/movement.hpp"
#include "settings.hpp"
#include <spdlog/spdlog.h>

namespace oo {

CharacterControllerImpl::CharacterControllerImpl(
    gsl::not_null<Ogre::SceneManager *> scnMgr,
    gsl::not_null<btDiscreteDynamicsWorld *> world)
    : mScnMgr(scnMgr), mWorld(world) {
  // TODO: Remove the camera from the implementation since it should be attached
  //       to the skeleton and only used for the player.
  static int counter{0};
  mCamera = mScnMgr->createCamera("__Camera" + std::to_string(counter++));

  mBodyNode = mScnMgr->getRootSceneNode()->createChildSceneNode();
  const auto bodyVec{qvm::convert_to<Ogre::Vector3>(
      qvm::_0X0(mHeight * 0.5f + getCapsuleRadius() * 0.5f))};
  mRootNode = mBodyNode->createChildSceneNode(-bodyVec);

  attachCamera(gsl::make_not_null(mCamera), gsl::make_not_null(mRootNode));
  createAndAttachRigidBody(gsl::make_not_null(mBodyNode));
}

CharacterControllerImpl::~CharacterControllerImpl() {
  if (mWorld && mRigidBody && mRigidBody->isInWorld()) {
    mWorld->removeRigidBody(mRigidBody.get());
  }

  if (mScnMgr) {
    // Destroying a node detaches all its attached objects and removes its
    // children, though it is not clear whether those children are deleted.
    if (mPitchNode) mScnMgr->destroySceneNode(mPitchNode);
    if (mCameraNode) mScnMgr->destroySceneNode(mCameraNode);
    if (mBodyNode) mScnMgr->destroySceneNode(mBodyNode);
    if (mRootNode) mScnMgr->destroySceneNode(mRootNode);
    if (mCamera) mScnMgr->destroyCamera(mCamera);
  }
}

CharacterControllerImpl::CharacterControllerImpl(CharacterControllerImpl &&other) noexcept {
  mScnMgr = std::exchange(other.mScnMgr, nullptr);
  mWorld = std::exchange(other.mWorld, nullptr);
  mCameraNode = std::exchange(other.mCameraNode, nullptr);
  mPitchNode = std::exchange(other.mPitchNode, nullptr);
  mCamera = std::exchange(other.mCamera, nullptr);
  mRootNode = std::exchange(other.mRootNode, nullptr);
  mBodyNode = std::exchange(other.mBodyNode, nullptr);
  mMotionState = std::exchange(other.mMotionState, nullptr);
  mCollisionShape = std::exchange(other.mCollisionShape, nullptr);
  mRigidBody = std::exchange(other.mRigidBody, nullptr);

  mSpeedAttribute = other.mSpeedAttribute;
  mAthleticsSkill = other.mAthleticsSkill;
  mAcrobaticsSkill = other.mAcrobaticsSkill;
  mRaceHeight = other.mRaceHeight;
  mWornWeight = other.mWornWeight;
  mHasWeaponOut = other.mHasWeaponOut;
  mIsRunning = other.mIsRunning;

  // mSpeedModifier is likely to capture `other` by reference, so we cannot
  // simply copy it, and since we don't know what state the CharacterController is
  // in we can't recreate it. It is up to the CharacterController to re-`enter()`
  // the current state.

  mHeight = other.mHeight;
  mMass = other.mMass;

  mPitch = std::move(other.mPitch);
  mYaw = std::move(other.mYaw);
  mLocalVelocity = std::move(other.mLocalVelocity);
}

CharacterControllerImpl &
CharacterControllerImpl::operator=(CharacterControllerImpl &&other) noexcept {
  if (this != &other) {
    mScnMgr = std::exchange(other.mScnMgr, nullptr);
    mWorld = std::exchange(other.mWorld, nullptr);
    mCameraNode = std::exchange(other.mCameraNode, nullptr);
    mPitchNode = std::exchange(other.mPitchNode, nullptr);
    mCamera = std::exchange(other.mCamera, nullptr);
    mRootNode = std::exchange(other.mRootNode, nullptr);
    mBodyNode = std::exchange(other.mBodyNode, nullptr);
    mMotionState = std::exchange(other.mMotionState, nullptr);
    mCollisionShape = std::exchange(other.mCollisionShape, nullptr);
    mRigidBody = std::exchange(other.mRigidBody, nullptr);

    mSpeedAttribute = other.mSpeedAttribute;
    mAthleticsSkill = other.mAthleticsSkill;
    mAcrobaticsSkill = other.mAcrobaticsSkill;
    mRaceHeight = other.mRaceHeight;
    mWornWeight = other.mWornWeight;
    mHasWeaponOut = other.mHasWeaponOut;
    mIsRunning = other.mIsRunning;

    // See move constructor for omitted mSpeedModifier

    mHeight = other.mHeight;
    mMass = other.mMass;

    mPitch = std::move(other.mPitch);
    mYaw = std::move(other.mYaw);
    mLocalVelocity = std::move(other.mLocalVelocity);
  }

  return *this;
}

void CharacterControllerImpl::attachCamera(gsl::not_null<Ogre::Camera *> camera,
                                           gsl::not_null<Ogre::SceneNode *> node) {
  const auto h{0.95f * mHeight};
  const auto camVec{qvm::convert_to<Ogre::Vector3>(qvm::_0X0(h))};
  mCameraNode = node->createChildSceneNode(camVec);
  mPitchNode = mCameraNode->createChildSceneNode();
  mPitchNode->attachObject(camera);
  // TODO: This does the right thing but is deprecated, use a base node to
  //       anchor the camera node to instead.
  camera->setPosition(Ogre::Vector3{0.0f, 0.2f, 0.2f});
}

void CharacterControllerImpl::createAndAttachRigidBody(gsl::not_null<Ogre::SceneNode *> node) {
  mMotionState = std::make_unique<Ogre::MotionState>(node);
  mCollisionShape = std::make_unique<btCapsuleShape>(getCapsuleRadius(),
                                                     getCapsuleHeight());
  btRigidBody::btRigidBodyConstructionInfo info(mMass,
                                                mMotionState.get(),
                                                mCollisionShape.get());
  mRigidBody = std::make_unique<btRigidBody>(info);
  mRigidBody->setAngularFactor(0.0f);

  const auto collisionLayer{bullet::CollisionLayer::OL_BIPED};
  const auto[group, mask]{bullet::getCollisionFilter(collisionLayer)};
  mWorld->addRigidBody(mRigidBody.get(), group, mask);
}

gsl::not_null<const btRigidBody *>
CharacterControllerImpl::getRigidBody() const noexcept {
  return gsl::make_not_null(mRigidBody.get());
}

gsl::not_null<btRigidBody *>
CharacterControllerImpl::getRigidBody() noexcept {
  return gsl::make_not_null(mRigidBody.get());
}

gsl::not_null<const Ogre::SceneNode *>
CharacterControllerImpl::getCameraNode() const noexcept {
  return gsl::make_not_null(mCameraNode);
}

gsl::not_null<Ogre::SceneNode *>
CharacterControllerImpl::getCameraNode() noexcept {
  return gsl::make_not_null(mCameraNode);
}

gsl::not_null<const Ogre::SceneNode *>
CharacterControllerImpl::getRootNode() const noexcept {
  return gsl::make_not_null(mRootNode);
}

gsl::not_null<Ogre::SceneNode *>
CharacterControllerImpl::getRootNode() noexcept {
  return gsl::make_not_null(mRootNode);
}

Ogre::Radian CharacterControllerImpl::getPitch() const noexcept {
  return mPitch;
}

Ogre::Radian &CharacterControllerImpl::getPitch() noexcept {
  return mPitch;
}

Ogre::Radian CharacterControllerImpl::getYaw() const noexcept {
  return mYaw;
}

Ogre::Radian &CharacterControllerImpl::getYaw() noexcept {
  return mYaw;
}

Ogre::Radian CharacterControllerImpl::getRootYaw() const noexcept {
  return mRootYaw;
}

Ogre::Radian &CharacterControllerImpl::getRootYaw() noexcept {
  return mRootYaw;
}

Ogre::Vector3 &CharacterControllerImpl::getLocalVelocity() noexcept {
  return mLocalVelocity;
}

Ogre::Vector3 CharacterControllerImpl::getLocalVelocity() const noexcept {
  return mLocalVelocity;
}

float CharacterControllerImpl::getSkill(oo::SkillIndex skill) const noexcept {
  switch (skill) {
    case SkillIndex::Athletics: return mAthleticsSkill;
    case SkillIndex::Acrobatics: return mAcrobaticsSkill;
    default: return 0.0f;
  }
}

float CharacterControllerImpl::getMass() const noexcept {
  return mMass;
}

bool CharacterControllerImpl::getIsRunning() const noexcept {
  return mIsRunning;
}

void CharacterControllerImpl::setIsRunning(bool isRunning) noexcept {
  mIsRunning = isRunning;
}

float CharacterControllerImpl::getHeight() const noexcept {
  return mHeight;
}

float CharacterControllerImpl::getMoveSpeed() const noexcept {
  const float base{oo::baseSpeed(mSpeedAttribute) * mRaceHeight
                       * oo::metersPerUnit<float>};
  const float weightMult{oo::encumbranceModifier(mWornWeight, mHasWeaponOut)};
  return base * weightMult
      * (mSpeedModifier ? mSpeedModifier(mHasWeaponOut, mIsRunning) : 1.0f);
}

float CharacterControllerImpl::getCapsuleRadius() const noexcept {
  return 0.3f;
}

float CharacterControllerImpl::getCapsuleHeight() const noexcept {
  return mHeight * 0.5f - getCapsuleRadius();
}

void CharacterControllerImpl::reactivatePhysics() noexcept {
  mRigidBody->activate(true);
}

void CharacterControllerImpl::updateCameraOrientation() noexcept {
  mCameraNode->setOrientation(Ogre::Quaternion(Ogre::Radian(0),
                                               Ogre::Vector3::UNIT_X));
  mRootNode->setOrientation(Ogre::Quaternion(Ogre::Radian(0),
                                             Ogre::Vector3::UNIT_X));
  mPitchNode->setOrientation(Ogre::Quaternion(Ogre::Radian(0),
                                              Ogre::Vector3::UNIT_X));

  mPitchNode->pitch(mPitch, Ogre::SceneNode::TS_LOCAL);
  mCameraNode->yaw(mYaw, Ogre::SceneNode::TS_LOCAL);
  mRootNode->yaw(mRootYaw, Ogre::SceneNode::TS_LOCAL);
}

void CharacterControllerImpl::move() noexcept {
  if (auto len = mLocalVelocity.length(); len > 0.01f) {
    // Camera and root yaw may not be aligned currently, but they should be when
    // the player is moving. Need to smooth camera yaw to zero while keeping the
    // absolute camera orientation and movement direction the same.
    // twistMultiplier should be in the range [0, 1), with higher values giving
    // slower body rotation times.
    // TODO: Make body twist time framerate independent.
    constexpr float twistMultiplier{0.75f};
    mRootYaw += mYaw * (1.0f - twistMultiplier);
    mYaw *= twistMultiplier;
    updateCameraOrientation();

    const auto speed{getMoveSpeed()};
    const auto rootAxes{mRootNode->getLocalAxes()};
    const auto cameraAxes{mCameraNode->getLocalAxes()};
    const auto axes{cameraAxes * rootAxes};

    const auto v{mRigidBody->getLinearVelocity()};
    auto newV{qvm::convert_to<btVector3>(axes * mLocalVelocity / len * speed)};
    newV.setY(v.y());
    mRigidBody->setLinearVelocity(newV);
  } else {
    const auto v{mRigidBody->getLinearVelocity()};
    mRigidBody->setLinearVelocity({0.0f, v.y(), 0.0f});
  }
}

void CharacterControllerImpl::setOrientation(Ogre::Radian pPitch,
                                             Ogre::Radian pYaw) noexcept {
  mPitch = pPitch;
  mRootYaw = pYaw;
  mYaw = Ogre::Radian{0.0f};
  updateCameraOrientation();
}

float CharacterControllerImpl::getSpringDisplacement() noexcept {
  const auto rayLength{10.0f};
  auto p0{qvm::convert_to<btVector3>(mMotionState->getPosition())};
  auto p1{qvm::convert_to<btVector3>(p0 + qvm::_0X0(-rayLength))};
  btCollisionWorld::AllHitsRayResultCallback callback(p0, p1);
  mWorld->rayTest(p0, p1, callback);

  const int numHits{callback.m_collisionObjects.size()};
  auto dist{rayLength};

  for (int i = 0; i < numHits; ++i) {
    const btCollisionObject *object{callback.m_collisionObjects[i]};
    if (object == mRigidBody.get()) continue;

    // No abs needed since p0 is above the collision point by construction.
    const auto distNew{p0.y() - callback.m_hitPointWorld[i].y()};
    if (distNew < dist) dist = distNew;
  }

  // Natural length of the spring
  const auto length{0.5f * mHeight + getCapsuleHeight() / 2.0f};

  return length - dist;
}

float CharacterControllerImpl::getMaxSpringDisplacement() noexcept {
  return 0.5f * mHeight - getCapsuleRadius();
}

void CharacterControllerImpl::applySpringForce(float displacement) noexcept {
  const float deltaMax{getMaxSpringDisplacement()};
  if (displacement < -deltaMax) return;

  const auto k0{4000.0f};
  const auto k1{700.0f};
  const auto v{mRigidBody->getLinearVelocity().y()};
  const auto f{k0 * displacement - k1 * v};
  mRigidBody->applyCentralForce(qvm::convert_to<btVector3>(qvm::_0X0(f)));
}

void CharacterControllerImpl::updatePhysics(float /*elapsed*/) noexcept {
  reactivatePhysics();
  updateCameraOrientation();
  move();
}

} // namespace oo
