#ifndef OPENOBLIVION_ENGINE_PLAYER_CONTROLLER_HPP
#define OPENOBLIVION_ENGINE_PLAYER_CONTROLLER_HPP

#include <Ogre.h>
#include <OgreVector3.h>
#include <OgreSceneNode.h>
#include <OgreCamera.h>
#include <array>

namespace engine {

class PlayerController {
 public:
  struct MoveEvent {
    enum Type : uint8_t {
      Left = 0,
      Right,
      Forward,
      Backward,
      Pitch,
      Yaw,
      N
    };
    Type type{};
    bool down{};
    float delta{};
    MoveEvent(Type type, bool down, float delta)
        : type(type), down(down), delta(delta) {}
  };

  explicit PlayerController(Ogre::SceneManager *scnMgr);

  Ogre::Camera *getCamera();

  Ogre::SceneNode *getCameraNode();

  void sendEvent(const MoveEvent &event);

  void moveTo(const Ogre::Vector3 &position);

  void update(float elapsed);

 private:
  const float speed{300.0f};
  Ogre::Radian pitch{0.0f};
  Ogre::Radian yaw{0.0f};
  Ogre::Vector3 localVelocity{Ogre::Vector3::ZERO};
  Ogre::SceneNode *cameraNode{};
  Ogre::SceneNode *pitchNode{};
  Ogre::Camera *camera{};
};

} // namespace engine

#endif // OPENOBLIVION_ENGINE_PLAYER_CONTROLLER_HPP
