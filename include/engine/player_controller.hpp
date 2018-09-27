#ifndef OPENOBLIVION_ENGINE_PLAYER_CONTROLLER_HPP
#define OPENOBLIVION_ENGINE_PLAYER_CONTROLLER_HPP

#include "engine/conversions.hpp"
#include "engine/ogre/motion_state.hpp"
#include "game_settings.hpp"
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
  GameSetting<float> fMoveCharWalkMin{"fMoveCharWalkMin", 90.0f};
  GameSetting<float> fMoveCharWalkMax{"fMoveCharWalkMax", 130.0f};
  GameSetting<float> fMoveRunMult{"fMoveRunMult", 3.0f};
  GameSetting<float> fMoveRunAthleticsMult{"fMoveRunAthleticsMult", 1.0f};
  float speedAttribute{50.0f};
  float athleticsSkill{50.0f};

  float moveTypeModifier(float athleticsSkill) {
    return *fMoveRunMult + *fMoveRunAthleticsMult * athleticsSkill * 0.01f;
  }

  float baseSpeed(float speedAttribute) {
    return *fMoveCharWalkMin
        + (*fMoveCharWalkMax - *fMoveCharWalkMin) * speedAttribute * 0.01f;
  }

  float speed(float speedAttribute, float athleticsSkill) {
    return moveTypeModifier(athleticsSkill) * baseSpeed(speedAttribute)
        * conversions::metersPerUnit<float>;
  }

  const float height = 1.0f * 128 * conversions::metersPerUnit<float>;

  Ogre::Radian pitch{0.0f};
  Ogre::Radian yaw{0.0f};
  Ogre::Vector3 localVelocity{Ogre::Vector3::ZERO};

  Ogre::SceneNode *cameraNode{};
  Ogre::SceneNode *pitchNode{};
  Ogre::Camera *camera{};

  Ogre::SceneNode *bodyNode{};
  std::unique_ptr<Ogre::MotionState> motionState{};
  std::unique_ptr<btCollisionShape> collisionShape{};
  std::unique_ptr<btRigidBody> rigidBody{};
};

} // namespace engine

#endif // OPENOBLIVION_ENGINE_PLAYER_CONTROLLER_HPP
