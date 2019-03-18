#include "character_controller/movement.hpp"
#include "character_controller/player_controller_impl.hpp"
#include "settings.hpp"
#include <spdlog/spdlog.h>

namespace oo {

PlayerControllerImpl::PlayerControllerImpl(
    gsl::not_null<Ogre::SceneManager *> scnMgr,
    gsl::not_null<btDiscreteDynamicsWorld *> world)
    : mScnMgr(scnMgr), mWorld(world) {
  mCamera = mScnMgr->createCamera("__PlayerCamera");
  mBodyNode = mScnMgr->getRootSceneNode()->createChildSceneNode();
  attachCamera(gsl::make_not_null(mCamera), gsl::make_not_null(mBodyNode));
  createAndAttachRigidBody(gsl::make_not_null(mBodyNode));
}

PlayerControllerImpl::~PlayerControllerImpl() {
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

void PlayerControllerImpl::attachCamera(gsl::not_null<Ogre::Camera *> camera,
                                        gsl::not_null<Ogre::SceneNode *> node) {
  const auto h{(0.95f - 0.5f) * height - getCapsuleHeight() / 2.0f};
  const auto camVec{qvm::convert_to<Ogre::Vector3>(qvm::_0X0(h))};
  mCameraNode = node->createChildSceneNode(camVec);
  mPitchNode = mCameraNode->createChildSceneNode();
  mPitchNode->attachObject(camera);
}

void PlayerControllerImpl::createAndAttachRigidBody(gsl::not_null<Ogre::SceneNode *> node) {
  mMotionState = std::make_unique<Ogre::MotionState>(node);
  mCollisionShape = std::make_unique<btCapsuleShape>(getCapsuleRadius(),
                                                     getCapsuleHeight());
  btRigidBody::btRigidBodyConstructionInfo info(mass,
                                                mMotionState.get(),
                                                mCollisionShape.get());
  mRigidBody = std::make_unique<btRigidBody>(info);
  mRigidBody->setAngularFactor(0.0f);
  mWorld->addRigidBody(mRigidBody.get());
}

gsl::not_null<const btRigidBody *>
PlayerControllerImpl::getRigidBody() const noexcept {
  return gsl::make_not_null(mRigidBody.get());
}

gsl::not_null<btRigidBody *>
PlayerControllerImpl::getRigidBody() noexcept {
  return gsl::make_not_null(mRigidBody.get());
}

gsl::not_null<const Ogre::SceneNode *>
PlayerControllerImpl::getCameraNode() const noexcept {
  return gsl::make_not_null(mCameraNode);
}

gsl::not_null<Ogre::SceneNode *>
PlayerControllerImpl::getCameraNode() noexcept {
  return gsl::make_not_null(mCameraNode);
}

float PlayerControllerImpl::getMoveSpeed() const noexcept {
  const float base{oo::baseSpeed(speedAttribute) * raceHeight
                       * oo::metersPerUnit<float>};
  const float weightMult{oo::encumbranceModifier(wornWeight, hasWeaponOut)};
  return base * weightMult
      * (mSpeedModifier ? mSpeedModifier(hasWeaponOut, isRunning) : 1.0f);
}

float PlayerControllerImpl::getCapsuleRadius() const noexcept {
  return 0.3f;
}

float PlayerControllerImpl::getCapsuleHeight() const noexcept {
  return height * 0.5f - getCapsuleRadius();
}

void PlayerControllerImpl::reactivatePhysics() noexcept {
  mRigidBody->activate(true);
}

void PlayerControllerImpl::updateCameraOrientation() noexcept {
  mCameraNode->setOrientation(Ogre::Quaternion(Ogre::Radian(0),
                                               Ogre::Vector3::UNIT_X));
  mPitchNode->setOrientation(Ogre::Quaternion(Ogre::Radian(0),
                                              Ogre::Vector3::UNIT_X));
  mPitchNode->pitch(pitch, Ogre::SceneNode::TS_LOCAL);
  mCameraNode->yaw(yaw, Ogre::SceneNode::TS_LOCAL);
}

void PlayerControllerImpl::move() noexcept {
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

float PlayerControllerImpl::getSpringDisplacement() noexcept {
  using namespace qvm;

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

float PlayerControllerImpl::getMaxSpringDisplacement() noexcept {
  return 0.5f * height - getCapsuleRadius();
}

void PlayerControllerImpl::applySpringForce(float displacement) noexcept {
  const float deltaMax{getMaxSpringDisplacement()};
  if (displacement < -deltaMax) return;

  const auto k0{4000.0f};
  const auto k1{700.0f};
  const auto v{mRigidBody->getLinearVelocity().y()};

  const auto &force{qvm::_0X0(k0 * displacement - k1 * v)};
  mRigidBody->applyCentralForce(qvm::convert_to<btVector3>(force));
}

void PlayerControllerImpl::updatePhysics(float /*elapsed*/) noexcept {
  reactivatePhysics();
  updateCameraOrientation();
  move();
}

} // namespace oo
