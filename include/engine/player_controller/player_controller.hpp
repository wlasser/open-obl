#ifndef OPENOBLIVION_ENGINE_PLAYER_CONTROLLER_HPP
#define OPENOBLIVION_ENGINE_PLAYER_CONTROLLER_HPP

#include "engine/controls.hpp"
#include "engine/conversions.hpp"
#include "engine/player_controller/abilities.hpp"
#include "engine/player_controller/player_jump_state.hpp"
#include "engine/player_controller/player_stand_state.hpp"
#include "engine/player_controller/player_state.hpp"
#include "ogrebullet/motion_state.hpp"
#include "game_settings.hpp"
#include <btBulletDynamicsCommon.h>
#include <Ogre.h>
#include <array>
#include <variant>

namespace engine {

class PlayerController {
 public:
  explicit PlayerController(Ogre::SceneManager *scnMgr);

  Ogre::Camera *getCamera();
  btRigidBody *getRigidBody();

  void handleEvent(const KeyVariant &event);
  void handleEvent(const MouseVariant &event);

  void update(float elapsed);

  void moveTo(const Ogre::Vector3 &position);

  void handleCollision(const btCollisionObject *other,
                       const btManifoldPoint &contact);

 private:
  friend PlayerState;
  friend class PlayerStandState;
  friend class PlayerJumpState;
  friend struct MoveAbility;
  friend struct LookAbility;
  std::shared_ptr<PlayerState> state{};

  GameSetting<float> fMoveCharWalkMin{"fMoveCharWalkMin", 90.0f};
  GameSetting<float> fMoveCharWalkMax{"fMoveCharWalkMax", 130.0f};
  GameSetting<float> fMoveRunMult{"fMoveRunMult", 3.0f};
  GameSetting<float> fMoveRunAthleticsMult{"fMoveRunAthleticsMult", 1.0f};
  GameSetting<float> fJumpHeightMin{"fJumpHeightMin", 64.0f};
  GameSetting<float> fJumpHeightMax{"fJumpHeightMax", 164.0f};
  float speedAttribute{50.0f};
  float athleticsSkill{50.0f};
  float acrobaticsSkill{50.0f};

  float moveTypeModifier(float athleticsSkill) const {
    return *fMoveRunMult + *fMoveRunAthleticsMult * athleticsSkill * 0.01f;
  }

  float baseSpeed(float speedAttribute) const {
    const float walkRange = *fMoveCharWalkMax - *fMoveCharWalkMax;
    return *fMoveCharWalkMin + walkRange * speedAttribute * 0.01f;
  }

  float speed(float speedAttribute, float athleticsSkill) const {
    return moveTypeModifier(athleticsSkill) * baseSpeed(speedAttribute)
        * conversions::metersPerUnit<float>;
  }

  float jumpHeight(float acrobaticsSkill) const {
    const float heightRange = *fJumpHeightMax - *fJumpHeightMin;
    return (*fJumpHeightMin + heightRange * acrobaticsSkill * 0.01f)
        * conversions::metersPerUnit<float>;
  }

  void updatePhysics(float elapsed);

  float height{1.0f * 128 * conversions::metersPerUnit<float>};
  float mass{80.0f};

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
