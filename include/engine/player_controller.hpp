#ifndef OPENOBLIVION_ENGINE_PLAYER_CONTROLLER_HPP
#define OPENOBLIVION_ENGINE_PLAYER_CONTROLLER_HPP

#include "engine/conversions.hpp"
#include "engine/ogre/motion_state.hpp"
#include <btBulletDynamicsCommon.h>
#include <Ogre.h>
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
  btRigidBody *getRigidBody();

  void sendEvent(const MoveEvent &event);

  void moveTo(const Ogre::Vector3 &position);

  void update(float elapsed);

 private:
  const float fMoveCharWalkMin{90.0f};
  const float fMoveCharWalkMax{130.0f};
  const float fMoveRunMult{3.0f};
  const float fMoveRunAthleticsMult{1.0f};
  float speedAttribute{50.0f};
  float athleticsSkill{50.0f};
  float moveTypeModifier
      {fMoveRunMult + fMoveRunAthleticsMult * athleticsSkill / 100.0f};
  float baseSpeed{fMoveCharWalkMin
                      + (fMoveCharWalkMax - fMoveCharWalkMin) * speedAttribute
                          / 100.0f};
  const float
      speed{moveTypeModifier * baseSpeed * conversions::metersPerUnit<float>};

  Ogre::Radian pitch{0.0f};
  Ogre::Radian yaw{0.0f};
  Ogre::Vector3 localVelocity{Ogre::Vector3::ZERO};

  Ogre::SceneNode *cameraNode{};
  Ogre::SceneNode *pitchNode{};
  Ogre::Camera *camera{};

  std::unique_ptr<Ogre::MotionState> motionState{};
  std::unique_ptr<btCollisionShape> collisionShape{};
  std::unique_ptr<btRigidBody> rigidBody{};
};

} // namespace engine

#endif // OPENOBLIVION_ENGINE_PLAYER_CONTROLLER_HPP
