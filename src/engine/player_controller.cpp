#include "engine/player_controller.hpp"

namespace engine {

PlayerController::PlayerController(Ogre::SceneManager *scnMgr, bool free)
    : free(free) {
  camera = scnMgr->createCamera("PlayerCamera");
  camera->setNearClipDistance(1.0f);
  camera->setAutoAspectRatio(true);

  cameraNode = scnMgr->getRootSceneNode()->createChildSceneNode();

  pitchNode = cameraNode->createChildSceneNode();
  pitchNode->attachObject(camera);

  if (!free) {
    motionState = std::make_unique<Ogre::MotionState>(cameraNode);
    collisionShape = std::make_unique<btCapsuleShape>(
        0.25f, 128.0f * conversions::unitsPerMeter<float>);
    btRigidBody::btRigidBodyConstructionInfo info(
        80.0f, motionState.get(), collisionShape.get());
    rigidBody = std::make_unique<btRigidBody>(info);
  }
}

Ogre::Camera *PlayerController::getCamera() {
  return camera;
}

Ogre::SceneNode *PlayerController::getCameraNode() {
  return cameraNode;
}

btRigidBody *PlayerController::getRigidBody() {
  if (free) return nullptr;
  else return rigidBody.get();
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
  cameraNode->setPosition(position);
  if (!free) motionState->notify();
}

void PlayerController::update(float elapsed) {
  if (free) {
    cameraNode->setOrientation(Ogre::Quaternion(Ogre::Radian(0),
                                                Ogre::Vector3::UNIT_X));
    pitchNode->setOrientation(Ogre::Quaternion(Ogre::Radian(0),
                                               Ogre::Vector3::UNIT_X));
    pitchNode->pitch(pitch, Ogre::SceneNode::TS_LOCAL);
    cameraNode->yaw(yaw, Ogre::SceneNode::TS_LOCAL);

    auto axes = cameraNode->getLocalAxes();
    // TODO: Is this normalization necessary or is axes already in SO(3)?
    axes = (1.0f / axes.determinant()) * axes;
    if (auto length = localVelocity.length() > 0.01f) {
      cameraNode->translate(axes,
                            localVelocity / length * speed * elapsed,
                            Ogre::SceneNode::TS_WORLD);
    }
  }
}

} // namespace engine
