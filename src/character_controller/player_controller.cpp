#include "character_controller/player_controller.hpp"
#include "settings.hpp"
#include <gsl/gsl>
#include <OgreMath.h>
#include <OgreSceneNode.h>
#include <spdlog/spdlog.h>
#include <cmath>
#include <optional>
#include <variant>

namespace oo {

PlayerController::PlayerController(gsl::not_null<Ogre::SceneManager *> scnMgr,
                                   gsl::not_null<btDiscreteDynamicsWorld *> world)
    : mImpl(scnMgr, world) {
  mState = StandState{};
  enter(mState);

  mMovementStateVariant = WalkState{};
  enter(mMovementStateVariant);
}

Ogre::Camera *PlayerController::getCamera() const noexcept {
  return mImpl.mCamera;
}

btRigidBody *PlayerController::getRigidBody() const noexcept {
  return mImpl.mRigidBody.get();
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
  mImpl.mBodyNode->setPosition(position);
  mImpl.mMotionState->notify();
  // Notifying the motionState is insufficient. We cannot force the
  // btRigidBody to update its transform, and must do it manually.
  btTransform trans{};
  mImpl.mMotionState->getWorldTransform(trans);
  mImpl.mRigidBody->setWorldTransform(trans);
}

void PlayerController::setOrientation(const Ogre::Quaternion &orientation) {
  mImpl.setOrientation(orientation.getPitch(), orientation.getYaw());
  mImpl.mMotionState->notify();
  // see moveTo()
  btTransform trans{};
  mImpl.mMotionState->getWorldTransform(trans);
  mImpl.mRigidBody->setWorldTransform(trans);
}

Ogre::Vector3 PlayerController::getPosition() const noexcept {
  return mImpl.mBodyNode->getPosition();
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
