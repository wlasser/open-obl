#include "engine/character_controller/player_controller.hpp"
#include "engine/settings.hpp"
#include <gsl/gsl>
#include <OgreMath.h>
#include <spdlog/spdlog.h>
#include <cmath>

namespace engine::character {

PlayerController::PlayerController(Ogre::SceneManager *scnMgr) {
  impl.camera = scnMgr->createCamera("PlayerCamera");
  auto camera{gsl::make_not_null(impl.camera)};
  setAspectRatio(camera);

  impl.bodyNode = scnMgr->getRootSceneNode()->createChildSceneNode();
  auto bodyNode{gsl::make_not_null(impl.bodyNode)};
  attachCamera(camera, bodyNode);
  createAndAttachRigidBody(bodyNode);

  state = StandState{};
  enter(state);

  movementState = WalkState{};
  enter(movementState);
}

Ogre::Camera *PlayerController::getCamera() const noexcept {
  return impl.camera;
}

btRigidBody *PlayerController::getRigidBody() const noexcept {
  return impl.rigidBody.get();
}

void PlayerController::handleEvent(const KeyVariant &event) {
  // @formatter:off
  auto newState{std::visit([this, &event](auto &&s) {
    return std::visit([this, &s](auto &&e) -> std::optional<StateVariant> {
      return s.handleEvent(impl, e);
      }, event);
    }, state)};
  // @formatter:on

  if (newState) changeState(*newState);

  auto newMovementState{std::visit([this, &event](auto &&s) {
    return std::visit([this, &s](auto &&e) -> std::optional<MovementStateVariant> {
      return s.handleEvent(impl, e);
    }, event);
  }, movementState)};

  if (newMovementState) changeState(*newMovementState);

}

void PlayerController::handleEvent(const MouseVariant &event) {
  std::visit([this, &event](auto &&s) {
    std::visit([this, &s](auto &&e) {
      s.handleEvent(impl, e);
    }, event);
  }, state);

  std::visit([this, &event](auto &&s) {
    std::visit([this, &s](auto &&e) {
      s.handleEvent(impl, e);
    }, event);
  }, movementState);
}

void PlayerController::update(float elapsed) {
  auto newState{std::visit(
      [this, elapsed](auto &&s) -> std::optional<StateVariant> {
        return s.update(impl, elapsed);
      }, state)};

  if (newState) changeState(*newState);

  auto newMovementState{std::visit(
      [this, elapsed](auto &&s) -> std::optional<MovementStateVariant> {
        return s.update(impl, elapsed);
      }, movementState)};

  if (newMovementState) changeState(*newMovementState);
}

void PlayerController::handleCollision(const btCollisionObject *other,
                                       const btManifoldPoint &contact) {
  auto newState{std::visit(
      [this, other, &contact](auto &&s) -> std::optional<StateVariant> {
        return s.handleCollision(impl, other, contact);
      }, state)};

  if (newState) changeState(*newState);

  auto newMovementState{std::visit(
      [this, other, &contact](auto &&s) -> std::optional<MovementStateVariant> {
        return s.handleCollision(impl, other, contact);
      }, movementState)};

  if (newMovementState) changeState(*newMovementState);
}

void PlayerController::moveTo(const Ogre::Vector3 &position) {
  impl.bodyNode->setPosition(position);
  impl.motionState->notify();
  // Notifying the motionState is insufficient. We cannot force the
  // btRigidBody to update its transform, and must do it manually.
  btTransform trans{};
  impl.motionState->getWorldTransform(trans);
  impl.rigidBody->setWorldTransform(trans);
}

void PlayerController::setAspectRatio(gsl::not_null<Ogre::Camera *> camera) const {
  const auto &settings{GameSettings::getSingleton()};
  const auto screenWidth
      {gsl::narrow_cast<float>(settings.iGet("Display.iSize W"))};
  const auto screenHeight
      {gsl::narrow_cast<float>(settings.iGet("Display.iSize H"))};

  const float aspectRatio{screenWidth / screenHeight};
  camera->setAspectRatio(aspectRatio);
  camera->setNearClipDistance(0.1f);

  // We are given the horizontal fov, but can only set the vertical fov.
  // Internally Ogre probably undoes this operation so this is inefficient and
  // possibly inaccurate.
  {
    using namespace Ogre;
    Degree xFov{settings.get<float>("Display.fDefaultFOV", 75.0f)};
    xFov = Math::Clamp(xFov.valueDegrees(), 1.0f, 179.0f);
    Degree yFov{2.0f * Math::ATan(1.0f / aspectRatio * Math::Tan(xFov / 2.0f))};
    camera->setFOVy(yFov);
  }
}

void PlayerController::attachCamera(gsl::not_null<Ogre::Camera *> camera,
                                    gsl::not_null<Ogre::SceneNode *> node) {
  impl.cameraNode = node->createChildSceneNode(
      Ogre::Vector3{0.0f, impl.height * 0.45f, 0.0f});
  impl.pitchNode = impl.cameraNode->createChildSceneNode();
  impl.pitchNode->attachObject(camera);
}

void PlayerController::createAndAttachRigidBody(gsl::not_null<Ogre::SceneNode *> node) {
  impl.motionState = std::make_unique<Ogre::MotionState>(node);
  const auto radius{0.3f};
  const auto height{impl.height - 2.0f * radius};
  impl.collisionShape = std::make_unique<btCapsuleShape>(radius, height);
  btRigidBody::btRigidBodyConstructionInfo info(impl.mass,
                                                impl.motionState.get(),
                                                impl.collisionShape.get());
  impl.rigidBody = std::make_unique<btRigidBody>(info);
  impl.rigidBody->setAngularFactor(0.0f);
}

void PlayerController::enter(StateVariant &state) {
  std::visit([this](auto &&s) { s.enter(this->impl); }, state);
}

void PlayerController::enter(MovementStateVariant &state) {
  std::visit([this](auto &&s) {
    s.enter(this->impl);
  }, state);
};

void PlayerController::exit(StateVariant &state) {
  std::visit([this](auto &&s) { s.exit(this->impl); }, state);
}

void PlayerController::exit(MovementStateVariant &state) {
  std::visit([this](auto &&s) { s.exit(this->impl); }, state);
}

void PlayerController::changeState(StateVariant newState) {
  exit(state);
  state = newState;
  enter(state);
}

void PlayerController::changeState(MovementStateVariant newState) {
  exit(movementState);
  movementState = newState;
  enter(movementState);
}

} // namespace engine::character
