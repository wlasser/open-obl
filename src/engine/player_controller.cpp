#include "engine/player_controller.hpp"

namespace engine {

PlayerController::PlayerController(Ogre::SceneManager *scnMgr) {
  camera = scnMgr->createCamera("PlayerCamera");
  camera->setNearClipDistance(1.0f);
  camera->setAutoAspectRatio(true);
  cameraNode = scnMgr->getRootSceneNode()->createChildSceneNode();
  pitchNode = cameraNode->createChildSceneNode();
  pitchNode->attachObject(camera);
}

Ogre::Camera *PlayerController::getCamera() {
  return camera;
}

Ogre::SceneNode *PlayerController::getCameraNode() {
  return cameraNode;
}

void PlayerController::sendEvent(const MoveEvent &event) {
  switch (event.type) {
    case MoveEvent::Left: {
      if (event.down) localVelocity.x -= speed;
      else localVelocity.x += speed;
      break;
    }
    case MoveEvent::Right: {
      if (event.down) localVelocity.x -= -speed;
      else localVelocity.x += -speed;
      break;
    }
    case MoveEvent::Forward: {
      if (event.down) localVelocity.z -= speed;
      else localVelocity.z += speed;
      break;
    }
    case MoveEvent::Backward: {
      if (event.down) localVelocity.z -= -speed * 2.0f / 3.0f;
      else localVelocity.z += -speed * 2.0f / 3.0f;
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
  cameraNode->setPosition(position);
}

void PlayerController::update(float elapsed) {
  cameraNode->setOrientation(Ogre::Quaternion(Ogre::Radian(0),
                                              Ogre::Vector3::UNIT_X));
  pitchNode->setOrientation(Ogre::Quaternion(Ogre::Radian(0),
                                             Ogre::Vector3::UNIT_X));
  pitchNode->pitch(pitch, Ogre::SceneNode::TS_LOCAL);
  cameraNode->yaw(yaw, Ogre::SceneNode::TS_LOCAL);

  auto axes = cameraNode->getLocalAxes();
  // TODO: Is this normalization necessary or is axes already in SO(3)?
  axes = (1.0f / axes.determinant()) * axes;
  cameraNode->translate(axes,
                        localVelocity * elapsed,
                        Ogre::SceneNode::TS_WORLD);
}

} // namespace engine
