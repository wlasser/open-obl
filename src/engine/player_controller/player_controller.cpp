#include "engine/player_controller/player_controller.hpp"
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

Ogre::Camera *PlayerController::getCamera() const noexcept {
  return camera;
}

btRigidBody *PlayerController::getRigidBody() const noexcept {
  return rigidBody.get();
}

void PlayerController::handleEvent(const KeyVariant &event) {
  auto newState{state ? state->handleEvent(this, event) : nullptr};
  if (newState) {
    state = std::move(newState);
    state->enter(this);
  }
}

void PlayerController::handleEvent(const MouseVariant &event) {
  if (state) state->handleEvent(this, event);
}

void PlayerController::update(float elapsed) {
  auto newState{state ? state->update(this, elapsed) : nullptr};
  if (newState) {
    state = std::move(newState);
    state->enter(this);
  }
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

  const auto speed{walkSpeed(speedAttribute, athleticsSkill, wornWeight,
                             raceHeight, hasWeaponOut)};

  // This is a rotation of the standard basis, so is still in SO(3)
  const auto axes{cameraNode->getLocalAxes()};
  if (auto length = localVelocity.length() > 0.01f) {
    const auto v{rigidBody->getLinearVelocity()};
    auto newV{Ogre::conversions::toBullet(
        axes * localVelocity / length * speed)};
    newV.setY(v.y());
    rigidBody->setLinearVelocity(newV);
  } else {
    const auto v{rigidBody->getLinearVelocity()};
    rigidBody->setLinearVelocity({0.0f, v.y(), 0.0f});
  }
}

} // namespace engine
