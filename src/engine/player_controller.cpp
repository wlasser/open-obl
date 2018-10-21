#include "engine/player_controller.hpp"
#include "engine/settings.hpp"
#include "ogrebullet/conversions.hpp"
#include <gsl/gsl>
#include <OgreMath.h>
#include <spdlog/spdlog.h>
#include <cmath>

namespace engine {

PlayerController::PlayerController(Ogre::SceneManager *scnMgr) {
  const auto &settings{GameSettings::getSingleton()};

  camera = scnMgr->createCamera("PlayerCamera");
  camera->setNearClipDistance(0.1f);

  const auto screenWidth
      {gsl::narrow_cast<float>(settings.iGet("Display.iSize W"))};
  const auto screenHeight
      {gsl::narrow_cast<float>(settings.iGet("Display.iSize H"))};

  camera->setAspectRatio(screenWidth / screenHeight);

  // We are given the horizontal fov, but can only set the vertical fov.
  // Internally Ogre probably undoes this operation so this is inefficient and
  // possibly inaccurate.
  Ogre::Degree xFov{settings.get<float>("Display.fDefaultFOV", 75.0f)};
  xFov = Ogre::Math::Clamp(xFov.valueDegrees(), 1.0f, 179.0f);
  Ogre::Degree yFov{2.0f * Ogre::Math::ATan(
      1.0f / camera->getAspectRatio() * Ogre::Math::Tan(xFov / 2.0f))};
  camera->setFOVy(yFov);

  bodyNode = scnMgr->getRootSceneNode()->createChildSceneNode();
  cameraNode = bodyNode->createChildSceneNode(
      Ogre::Vector3{0.0f, height * 0.45f, 0.0f});

  pitchNode = cameraNode->createChildSceneNode();
  pitchNode->attachObject(camera);

  motionState = std::make_unique<Ogre::MotionState>(bodyNode);
  const auto radius{0.3f};
  collisionShape = std::make_unique<btCapsuleShape>(radius,
                                                    height - 2.0f * radius);
  btRigidBody::btRigidBodyConstructionInfo info(mass,
                                                motionState.get(),
                                                collisionShape.get());
  rigidBody = std::make_unique<btRigidBody>(info);
  rigidBody->setAngularFactor(0.0f);

  state = std::make_shared<PlayerStandState>();
}

Ogre::Camera *PlayerController::getCamera() {
  return camera;
}

btRigidBody *PlayerController::getRigidBody() {
  return rigidBody.get();
}

void PlayerController::handleEvent(const MoveEvent &event) {
  auto newState{state ? state->handleEvent(this, event) : nullptr};
  if (newState) {
    state = std::move(newState);
    state->enter(this);
  }
}

void PlayerController::update(float elapsed) {
  auto newState{state ? state->update(this, elapsed) : nullptr};
  if (newState) {
    state = std::move(newState);
    state->enter(this);
  }
}

bool PlayerState::handleMove(PlayerController *player, const MoveEvent &event) {
  switch (event.type) {
    case MoveEvent::Left: {
      player->localVelocity.x -= (event.down ? 1.0f : -1.0f);
      return true;
    }
    case MoveEvent::Right: {
      player->localVelocity.x += (event.down ? 1.0f : -1.0f);
      return true;
    }
    case MoveEvent::Forward: {
      player->localVelocity.z -= (event.down ? 1.0f : -1.0f);
      return true;
    }
    case MoveEvent::Backward: {
      player->localVelocity.z += (event.down ? 1.0f : -1.0f);
      return true;
    }
    default: return false;
  }
}

bool PlayerState::handleLook(PlayerController *player, const MoveEvent &event) {
  switch (event.type) {
    case MoveEvent::Pitch: {
      player->pitch += -Ogre::Radian(event.delta / 750.0f);
      return true;
    }
    case MoveEvent::Yaw: {
      player->yaw += -Ogre::Radian(event.delta / 750.0f);
      return true;
    }
    default: return false;
  }
}

std::shared_ptr<PlayerState>
PlayerStandState::handleEvent(PlayerController *player,
                              const MoveEvent &event) {
  if (PlayerState::handleMove(player, event)) return nullptr;
  if (PlayerState::handleLook(player, event)) return nullptr;
  if (event.type == MoveEvent::Jump) {
    return std::make_shared<PlayerJumpState>();
  }
  return nullptr;
}

std::shared_ptr<PlayerState>
PlayerStandState::update(PlayerController *player, float elapsed) {
  player->updatePhysics(elapsed);
  return nullptr;
}

std::shared_ptr<PlayerState>
PlayerJumpState::handleEvent(PlayerController *player, const MoveEvent &event) {
  if (PlayerState::handleMove(player, event)) return nullptr;
  if (PlayerState::handleLook(player, event)) return nullptr;
  return nullptr;
}

std::shared_ptr<PlayerState>
PlayerJumpState::update(PlayerController *player, float elapsed) {
  player->updatePhysics(elapsed);
  return nullptr;
}

void PlayerJumpState::enter(PlayerController *player) {
  // Player jumps in the opposite direction of gravity, with an impulse chosen
  // to give the desired jump height. To find the impulse, use v^2 = u^2 + 2as
  // along with the fact that the impulse is the change in momentum.
  const btVector3 gravityVector{player->rigidBody->getGravity()};
  const float g{gravityVector.length()};
  const float apex{player->jumpHeight(player->acrobaticsSkill)};
  const float impulse{player->mass * std::sqrt(2.0f * g * apex)};
  player->rigidBody->applyCentralImpulse(-impulse * gravityVector.normalized());
}

std::shared_ptr<PlayerState>
PlayerJumpState::handleCollision(PlayerController *player,
                                 const btCollisionObject *other,
                                 const btManifoldPoint &contact) {
  const auto impulse{contact.getAppliedImpulse()};
  const auto r{contact.getPositionWorldOnA() - contact.getPositionWorldOnB()};
  spdlog::get(settings::log)->info("Player received of impulse {} N", impulse);
  if (r.normalized().dot(player->rigidBody->getGravity().normalized()) > 0.7) {
    return std::make_shared<PlayerStandState>();
  }
  return nullptr;
}

void PlayerController::moveTo(const Ogre::Vector3 &position) {
  bodyNode->setPosition(position);
  motionState->notify();
  // Notifying the motionState is insufficient. We cannot force the
  // btRigidBody to update its transform, and must do it manually.
  btTransform trans{};
  motionState->getWorldTransform(trans);
  rigidBody->setWorldTransform(trans);
}

void PlayerController::handleCollision(const btCollisionObject *other,
                                       const btManifoldPoint &contact) {
  auto newState{state ? state->handleCollision(this, other, contact) : nullptr};
  if (newState) {
    state = std::move(newState);
    state->enter(this);
  }
}

void PlayerController::updatePhysics(float elapsed) {
  rigidBody->activate(true);
  cameraNode->setOrientation(Ogre::Quaternion(Ogre::Radian(0),
                                              Ogre::Vector3::UNIT_X));
  pitchNode->setOrientation(Ogre::Quaternion(Ogre::Radian(0),
                                             Ogre::Vector3::UNIT_X));
  pitchNode->pitch(pitch, Ogre::SceneNode::TS_LOCAL);
  cameraNode->yaw(yaw, Ogre::SceneNode::TS_LOCAL);

  // This is a rotation of the standard basis, so is still in SO(3)
  const auto axes{cameraNode->getLocalAxes()};
  if (auto length = localVelocity.length() > 0.01f) {
    const auto v{rigidBody->getLinearVelocity()};
    auto newV{Ogre::conversions::toBullet(
        axes * localVelocity / length * speed(speedAttribute, athleticsSkill))};
    newV.setY(v.y());
    rigidBody->setLinearVelocity(newV);
  } else {
    const auto v{rigidBody->getLinearVelocity()};
    rigidBody->setLinearVelocity({0.0f, v.y(), 0.0f});
  }
}

} // namespace engine
