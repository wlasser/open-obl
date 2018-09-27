#include "engine/player_controller.hpp"
#include "engine/settings.hpp"
#include <OgreMath.h>

namespace engine {

PlayerController::PlayerController(Ogre::SceneManager *scnMgr) {
  auto &settings = GameSettings::getSingleton();

  camera = scnMgr->createCamera("PlayerCamera");
  camera->setNearClipDistance(0.1f);

  float screenWidth = settings.iGet("Display.iSize W");
  float screenHeight = settings.iGet("Display.iSize H");
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
  collisionShape = std::make_unique<btCapsuleShape>(0.30f, height);
  btRigidBody::btRigidBodyConstructionInfo info(
      80.0f, motionState.get(), collisionShape.get());
  rigidBody = std::make_unique<btRigidBody>(info);
  rigidBody->setAngularFactor(0.0f);
}

Ogre::Camera *PlayerController::getCamera() {
  return camera;
}

Ogre::SceneNode *PlayerController::getCameraNode() {
  return cameraNode;
}

btRigidBody *PlayerController::getRigidBody() {
  return rigidBody.get();
}

void PlayerController::sendEvent(const MoveEvent &event) {
  switch (event.type) {
    case MoveEvent::Left: {
      if (event.down) localVelocity.x -= 1.0f;
      else localVelocity.x += 1.0f;
      break;
    }
    case MoveEvent::Right: {
      if (event.down) localVelocity.x -= -1.0f;
      else localVelocity.x += -1.0f;
      break;
    }
    case MoveEvent::Forward: {
      if (event.down) localVelocity.z -= 1.0f;
      else localVelocity.z += 1.0f;
      break;
    }
    case MoveEvent::Backward: {
      if (event.down) localVelocity.z -= -1.0f;
      else localVelocity.z += -1.0f;
      break;
    }
    case MoveEvent::Pitch: {
      pitch += -Ogre::Radian(event.delta / 750.0f);
      break;
    }
    case MoveEvent::Yaw: {
      yaw += -Ogre::Radian(event.delta / 750.0f);
      break;
    }
    case MoveEvent::N:break;
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

void PlayerController::update(float elapsed) {
  rigidBody->activate(true);
  cameraNode->setOrientation(Ogre::Quaternion(Ogre::Radian(0),
                                              Ogre::Vector3::UNIT_X));
  pitchNode->setOrientation(Ogre::Quaternion(Ogre::Radian(0),
                                             Ogre::Vector3::UNIT_X));
  pitchNode->pitch(pitch, Ogre::SceneNode::TS_LOCAL);
  cameraNode->yaw(yaw, Ogre::SceneNode::TS_LOCAL);

  // This is a rotation of the standard basis, so is still in SO(3)
  auto axes = cameraNode->getLocalAxes();
  if (auto length = localVelocity.length() > 0.01f) {
    auto v = rigidBody->getLinearVelocity();
    auto newV = conversions::toBullet(
        axes * localVelocity / length * speed(speedAttribute, athleticsSkill));
    newV.setY(v.y());
    rigidBody->setLinearVelocity(newV);
  } else {
    auto v = rigidBody->getLinearVelocity();
    rigidBody->setLinearVelocity({0.0f, v.y(), 0.0f});
  }
}

} // namespace engine
