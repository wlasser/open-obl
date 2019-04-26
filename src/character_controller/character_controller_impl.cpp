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
  attachCamera(gsl::make_not_null(mCamera), gsl::make_not_null(mBodyNode));
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
    if (mCamera) mScnMgr->destroyCamera(mCamera);
  }
}

CharacterControllerImpl::CharacterControllerImpl(CharacterControllerImpl &&other) noexcept {
  mScnMgr = std::exchange(other.mScnMgr, nullptr);
  mWorld = std::exchange(other.mWorld, nullptr);
  mCameraNode = std::exchange(other.mCameraNode, nullptr);
  mPitchNode = std::exchange(other.mPitchNode, nullptr);
  mCamera = std::exchange(other.mCamera, nullptr);
  mBodyNode = std::exchange(other.mBodyNode, nullptr);
  mMotionState = std::exchange(other.mMotionState, nullptr);
  mCollisionShape = std::exchange(other.mCollisionShape, nullptr);
  mRigidBody = std::exchange(other.mRigidBody, nullptr);

  speedAttribute = other.speedAttribute;
  athleticsSkill = other.athleticsSkill;
  acrobaticsSkill = other.acrobaticsSkill;
  raceHeight = other.raceHeight;
  wornWeight = other.wornWeight;
  hasWeaponOut = other.hasWeaponOut;
  isRunning = other.isRunning;

  // mSpeedModifier is likely to capture `other` by reference, so we cannot
  // simply copy it, and since we don't know what state the CharacterController is
  // in we can't recreate it. It is up to the CharacterController to re-`enter()`
  // the current state.

  height = other.height;
  mass = other.mass;

  pitch = std::move(other.pitch);
  yaw = std::move(other.yaw);
  localVelocity = std::move(other.localVelocity);
}

CharacterControllerImpl &
CharacterControllerImpl::operator=(CharacterControllerImpl &&other) noexcept {
  if (this != &other) {
    mScnMgr = std::exchange(other.mScnMgr, nullptr);
    mWorld = std::exchange(other.mWorld, nullptr);
    mCameraNode = std::exchange(other.mCameraNode, nullptr);
    mPitchNode = std::exchange(other.mPitchNode, nullptr);
    mCamera = std::exchange(other.mCamera, nullptr);
    mBodyNode = std::exchange(other.mBodyNode, nullptr);
    mMotionState = std::exchange(other.mMotionState, nullptr);
    mCollisionShape = std::exchange(other.mCollisionShape, nullptr);
    mRigidBody = std::exchange(other.mRigidBody, nullptr);

    speedAttribute = other.speedAttribute;
    athleticsSkill = other.athleticsSkill;
    acrobaticsSkill = other.acrobaticsSkill;
    raceHeight = other.raceHeight;
    wornWeight = other.wornWeight;
    hasWeaponOut = other.hasWeaponOut;
    isRunning = other.isRunning;

    // See move constructor for omitted mSpeedModifier

    height = other.height;
    mass = other.mass;

    pitch = std::move(other.pitch);
    yaw = std::move(other.yaw);
    localVelocity = std::move(other.localVelocity);
  }

  return *this;
}

void CharacterControllerImpl::attachCamera(gsl::not_null<Ogre::Camera *> camera,
                                           gsl::not_null<Ogre::SceneNode *> node) {
  const auto h{(0.95f - 0.5f) * height - getCapsuleHeight() / 2.0f};
  const auto camVec{qvm::convert_to<Ogre::Vector3>(qvm::_0X0(h))};
  mCameraNode = node->createChildSceneNode(camVec);
  mPitchNode = mCameraNode->createChildSceneNode();
  mPitchNode->attachObject(camera);
  camera->setPosition(Ogre::Vector3{0.0f, 1.0f, 3.0f});
}

void CharacterControllerImpl::createAndAttachRigidBody(gsl::not_null<Ogre::SceneNode *> node) {
  mMotionState = std::make_unique<Ogre::MotionState>(node);
  mCollisionShape = std::make_unique<btCapsuleShape>(getCapsuleRadius(),
                                                     getCapsuleHeight());
  btRigidBody::btRigidBodyConstructionInfo info(mass,
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
CharacterControllerImpl::getBodyNode() const noexcept {
  return gsl::make_not_null(mBodyNode);
}

gsl::not_null<Ogre::SceneNode *>
CharacterControllerImpl::getBodyNode() noexcept {
  return gsl::make_not_null(mBodyNode);
}

float CharacterControllerImpl::getMoveSpeed() const noexcept {
  const float base{oo::baseSpeed(speedAttribute) * raceHeight
                       * oo::metersPerUnit<float>};
  const float weightMult{oo::encumbranceModifier(wornWeight, hasWeaponOut)};
  return base * weightMult
      * (mSpeedModifier ? mSpeedModifier(hasWeaponOut, isRunning) : 1.0f);
}

float CharacterControllerImpl::getCapsuleRadius() const noexcept {
  return 0.3f;
}

float CharacterControllerImpl::getCapsuleHeight() const noexcept {
  return height * 0.5f - getCapsuleRadius();
}

void CharacterControllerImpl::reactivatePhysics() noexcept {
  mRigidBody->activate(true);
}

void CharacterControllerImpl::updateCameraOrientation() noexcept {
  mCameraNode->setOrientation(Ogre::Quaternion(Ogre::Radian(0),
                                               Ogre::Vector3::UNIT_X));
  mPitchNode->setOrientation(Ogre::Quaternion(Ogre::Radian(0),
                                              Ogre::Vector3::UNIT_X));
  mPitchNode->pitch(pitch, Ogre::SceneNode::TS_LOCAL);
  mCameraNode->yaw(yaw, Ogre::SceneNode::TS_LOCAL);
}

void CharacterControllerImpl::move() noexcept {
  const auto speed{getMoveSpeed()};
  // This is a rotation of the standard basis, so is still in SO(3)
  const auto axes{mCameraNode->getLocalAxes()};
  if (auto len = localVelocity.length(); len > 0.01f) {
    const auto v{mRigidBody->getLinearVelocity()};
    auto newV{qvm::convert_to<btVector3>(axes * localVelocity / len * speed)};
    newV.setY(v.y());
    mRigidBody->setLinearVelocity(newV);
  } else {
    const auto v{mRigidBody->getLinearVelocity()};
    mRigidBody->setLinearVelocity({0.0f, v.y(), 0.0f});
  }
}

void CharacterControllerImpl::setOrientation(Ogre::Radian pPitch,
                                             Ogre::Radian pYaw) noexcept {
  pitch = pPitch;
  yaw = pYaw;
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
  const auto length{0.5f * height + getCapsuleHeight() / 2.0f};

  return length - dist;
}

float CharacterControllerImpl::getMaxSpringDisplacement() noexcept {
  return 0.5f * height - getCapsuleRadius();
}

void CharacterControllerImpl::applySpringForce(float displacement) noexcept {
  const float deltaMax{getMaxSpringDisplacement()};
  if (displacement < -deltaMax) return;

  const auto k0{4000.0f};
  const auto k1{700.0f};
  const auto v{mRigidBody->getLinearVelocity().y()};

  const auto &force{qvm::_0X0(k0 * displacement - k1 * v)};
  mRigidBody->applyCentralForce(qvm::convert_to<btVector3>(force));
}

void CharacterControllerImpl::updatePhysics(float /*elapsed*/) noexcept {
  reactivatePhysics();
  updateCameraOrientation();
  move();
}

} // namespace oo
