#include "engine/character_controller/player_controller.hpp"
#include "engine/settings.hpp"
#include <gsl/gsl>
#include <OgreMath.h>
#include <spdlog/spdlog.h>
#include <cmath>

namespace engine::character {

PlayerController::PlayerController(Ogre::SceneManager *scnMgr) {
  const auto &settings{GameSettings::getSingleton()};

  impl.camera = scnMgr->createCamera("PlayerCamera");
  impl.camera->setNearClipDistance(0.1f);

  const auto screenWidth
      {gsl::narrow_cast<float>(settings.iGet("Display.iSize W"))};
  const auto screenHeight
      {gsl::narrow_cast<float>(settings.iGet("Display.iSize H"))};

  impl.camera->setAspectRatio(screenWidth / screenHeight);

  // We are given the horizontal fov, but can only set the vertical fov.
  // Internally Ogre probably undoes this operation so this is inefficient and
  // possibly inaccurate.
  Ogre::Degree xFov{settings.get<float>("Display.fDefaultFOV", 75.0f)};
  xFov = Ogre::Math::Clamp(xFov.valueDegrees(), 1.0f, 179.0f);
  Ogre::Degree yFov{2.0f * Ogre::Math::ATan(
      1.0f / impl.camera->getAspectRatio() * Ogre::Math::Tan(xFov / 2.0f))};
  impl.camera->setFOVy(yFov);

  impl.bodyNode = scnMgr->getRootSceneNode()->createChildSceneNode();
  impl.cameraNode = impl.bodyNode->createChildSceneNode(
      Ogre::Vector3{0.0f, impl.height * 0.45f, 0.0f});

  impl.pitchNode = impl.cameraNode->createChildSceneNode();
  impl.pitchNode->attachObject(impl.camera);

  impl.motionState = std::make_unique<Ogre::MotionState>(impl.bodyNode);
  const auto radius{0.3f};
  impl.collisionShape =
      std::make_unique<btCapsuleShape>(radius, impl.height - 2.0f * radius);
  btRigidBody::btRigidBodyConstructionInfo info(impl.mass,
                                                impl.motionState.get(),
                                                impl.collisionShape.get());
  impl.rigidBody = std::make_unique<btRigidBody>(info);
  impl.rigidBody->setAngularFactor(0.0f);

  state = StandState{};
  std::visit([this](auto &&s) { s.enter(impl); }, state);
}

Ogre::Camera *PlayerController::getCamera() const noexcept {
  return impl.camera;
}

btRigidBody *PlayerController::getRigidBody() const noexcept {
  return impl.rigidBody.get();
}

void PlayerController::handleEvent(const KeyVariant &event) {
  auto newState{std::visit(
      [this, &event](auto &&s) -> std::optional<StateVariant> {
        return std::visit(
            [this, &s](auto &&e) -> std::optional<StateVariant> {
              return liftOptional<StateVariant>(s.handleEvent(impl, e));
            },
            event);
      },
      state)};

  if (newState) {
    std::visit([this](auto &&s) { s.exit(impl); }, state);
    state = std::move(*newState);
    std::visit([this](auto &&s) { s.enter(impl); }, state);
  }
}

void PlayerController::handleEvent(const MouseVariant &event) {
  std::visit([this, &event](auto &&s) {
    std::visit([this, &s](auto &&e) {
      s.handleEvent(impl, e);
    }, event);
  }, state);
}

void PlayerController::update(float elapsed) {
  auto newState{std::visit(
      [this, elapsed](auto &&s) -> std::optional<StateVariant> {
        return liftOptional<StateVariant>(s.update(impl, elapsed));
      }, state)};

  if (newState) {
    std::visit([this](auto &&s) { s.exit(impl); }, state);
    state = std::move(*newState);
    std::visit([this](auto &&s) { s.enter(impl); }, state);
  }
}

void PlayerController::handleCollision(const btCollisionObject *other,
                                       const btManifoldPoint &contact) {
  auto newState{std::visit(
      [this, other, &contact](auto &&s) -> std::optional<StateVariant> {
        return s.handleCollision(impl, other, contact);
      }, state)};

  if (newState) {
    std::visit([this](auto &&s) { s.exit(impl); }, state);
    state = std::move(*newState);
    std::visit([this](auto &&s) { s.enter(impl); }, state);
  }
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

} // namespace engine::character
