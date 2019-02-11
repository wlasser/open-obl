#include "character_controller/player_controller.hpp"
#include "settings.hpp"
#include <gsl/gsl>
#include <OgreMath.h>
#include <spdlog/spdlog.h>
#include <cmath>
#include <optional>
#include <variant>

namespace oo {

PlayerController::PlayerController(Ogre::SceneManager *scnMgr,
                                   btDiscreteDynamicsWorld *world) {
  mImpl.camera = scnMgr->createCamera("PlayerCamera");
  auto camera{gsl::make_not_null(mImpl.camera)};

  mImpl.bodyNode = scnMgr->getRootSceneNode()->createChildSceneNode();
  auto bodyNode{gsl::make_not_null(mImpl.bodyNode)};
  attachCamera(camera, bodyNode);
  createAndAttachRigidBody(bodyNode);

  if (world) {
    mImpl.world = world;
    mImpl.world->addRigidBody(getRigidBody());
  }

  mState = StandState{};
  enter(mState);

  mMovementStateVariant = WalkState{};
  enter(mMovementStateVariant);
}

Ogre::Camera *PlayerController::getCamera() const noexcept {
  return mImpl.camera;
}

btRigidBody *PlayerController::getRigidBody() const noexcept {
  return mImpl.rigidBody.get();
}

void PlayerController::handleEvent(const KeyVariant &event) {
  auto newState{std::visit([this, &event](auto &&s) {
    return std::visit([this, &s](auto &&e) -> std::optional<StateVariant> {
      return s.handleEvent(mImpl, e);
    }, event);
  }, mState)};

  if (newState) changeState(*newState);

  auto newMovementState{std::visit([this, &event](auto &&s) {
    return std::visit([this, &s](auto &&e) -> std::optional<MovementStateVariant> {
      return s.handleEvent(mImpl, e);
    }, event);
  }, mMovementStateVariant)};

  if (newMovementState) changeState(*newMovementState);
}

void PlayerController::handleEvent(const MouseVariant &event) {
  std::visit([this, &event](auto &&s) {
    std::visit([this, &s](auto &&e) { s.handleEvent(mImpl, e); }, event);
  }, mState);

  std::visit([this, &event](auto &&s) {
    std::visit([this, &s](auto &&e) { s.handleEvent(mImpl, e); }, event);
  }, mMovementStateVariant);
}

void PlayerController::update(float elapsed) {
  auto newState{std::visit(
      [this, elapsed](auto &&s) -> std::optional<StateVariant> {
        return s.update(mImpl, elapsed);
      }, mState)};

  if (newState) changeState(*newState);

  auto newMovementState{std::visit(
      [this, elapsed](auto &&s) -> std::optional<MovementStateVariant> {
        return s.update(mImpl, elapsed);
      }, mMovementStateVariant)};

  if (newMovementState) changeState(*newMovementState);
}

void PlayerController::handleCollision(const btCollisionObject *other,
                                       const btManifoldPoint &contact) {
  auto newState{std::visit(
      [this, other, &contact](auto &&s) -> std::optional<StateVariant> {
        return s.handleCollision(mImpl, other, contact);
      }, mState)};

  if (newState) changeState(*newState);

  auto newMovementState{std::visit(
      [this, other, &contact](auto &&s) -> std::optional<MovementStateVariant> {
        return s.handleCollision(mImpl, other, contact);
      }, mMovementStateVariant)};

  if (newMovementState) changeState(*newMovementState);
}

void PlayerController::moveTo(const Ogre::Vector3 &position) {
  mImpl.bodyNode->setPosition(position);
  mImpl.motionState->notify();
  // Notifying the motionState is insufficient. We cannot force the
  // btRigidBody to update its transform, and must do it manually.
  btTransform trans{};
  mImpl.motionState->getWorldTransform(trans);
  mImpl.rigidBody->setWorldTransform(trans);
}

void PlayerController::attachCamera(gsl::not_null<Ogre::Camera *> camera,
                                    gsl::not_null<Ogre::SceneNode *> node) {
  const auto h{(0.95f - 0.5f) * mImpl.height - mImpl.getCapsuleHeight() / 2.0f};
  const auto camVector{qvm::convert_to<Ogre::Vector3>(qvm::_0X0(h))};
  mImpl.cameraNode = node->createChildSceneNode(camVector);
  mImpl.pitchNode = mImpl.cameraNode->createChildSceneNode();
  mImpl.pitchNode->attachObject(camera);
}

void PlayerController::createAndAttachRigidBody(gsl::not_null<Ogre::SceneNode *> node) {
  mImpl.motionState = std::make_unique<Ogre::MotionState>(node);

  const float r{mImpl.getCapsuleRadius()}, h{mImpl.getCapsuleHeight()};
  mImpl.collisionShape = std::make_unique<btCapsuleShape>(r, h);

  btRigidBody::btRigidBodyConstructionInfo info(mImpl.mass,
                                                mImpl.motionState.get(),
                                                mImpl.collisionShape.get());
  mImpl.rigidBody = std::make_unique<btRigidBody>(info);
  mImpl.rigidBody->setAngularFactor(0.0f);
}

void PlayerController::enter(StateVariant &state) {
  std::visit([this](auto &&s) { s.enter(this->mImpl); }, state);
}

void PlayerController::enter(MovementStateVariant &state) {
  std::visit([this](auto &&s) {
    s.enter(this->mImpl);
  }, state);
};

void PlayerController::exit(StateVariant &state) {
  std::visit([this](auto &&s) { s.exit(this->mImpl); }, state);
}

void PlayerController::exit(MovementStateVariant &state) {
  std::visit([this](auto &&s) { s.exit(this->mImpl); }, state);
}

void PlayerController::changeState(StateVariant newState) {
  exit(mState);
  mState = newState;
  enter(mState);
}

void PlayerController::changeState(MovementStateVariant newState) {
  exit(mMovementStateVariant);
  mMovementStateVariant = newState;
  enter(mMovementStateVariant);
}

} // namespace oo
