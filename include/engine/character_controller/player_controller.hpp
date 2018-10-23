#ifndef OPENOBLIVION_ENGINE_CHARACTER_CONTROLLER_PLAYER_CONTROLLER_HPP
#define OPENOBLIVION_ENGINE_CHARACTER_CONTROLLER_PLAYER_CONTROLLER_HPP

#include "engine/controls.hpp"
#include "engine/conversions.hpp"
#include "engine/character_controller/player_controller_impl.hpp"
#include "engine/character_controller/fallback_state.hpp"
#include "engine/character_controller/jump_state.hpp"
#include "engine/character_controller/run_state.hpp"
#include "engine/character_controller/sneak_jump_state.hpp"
#include "engine/character_controller/sneak_stand_state.hpp"
#include "engine/character_controller/stand_state.hpp"
#include "engine/character_controller/walk_state.hpp"
#include "ogrebullet/motion_state.hpp"
#include "game_settings.hpp"
#include <btBulletDynamicsCommon.h>
#include <gsl/gsl>
#include <Ogre.h>
#include <variant>

namespace engine::character {

using StateVariant = std::variant<StandState,
                                  JumpState,
                                  SneakStandState,
                                  SneakJumpState>;
using MovementStateVariant = std::variant<WalkState, RunState>;

class PlayerController {
 public:
  explicit PlayerController(Ogre::SceneManager *scnMgr);

  Ogre::Camera *getCamera() const noexcept;
  btRigidBody *getRigidBody() const noexcept;

  void handleEvent(const KeyVariant &event);
  void handleEvent(const MouseVariant &event);

  void update(float elapsed);

  void moveTo(const Ogre::Vector3 &position);

  void handleCollision(const btCollisionObject *other,
                       const btManifoldPoint &contact);

 private:
  PlayerControllerImpl impl{};
  StateVariant state{};
  MovementStateVariant movementState{};

  void setAspectRatio(gsl::not_null<Ogre::Camera *> camera) const;
  void attachCamera(gsl::not_null<Ogre::Camera *> camera,
                    gsl::not_null<Ogre::SceneNode *> node);
  void createAndAttachRigidBody(gsl::not_null<Ogre::SceneNode *> node);

  void enter(StateVariant &state);
  void enter(MovementStateVariant &state);
  void exit(StateVariant &state);
  void exit(MovementStateVariant &state);
  void changeState(StateVariant newState);
  void changeState(MovementStateVariant newState);
};

} // namespace engine::character

#endif // OPENOBLIVION_ENGINE_CHARACTER_CONTROLLER_PLAYER_CONTROLLER_HPP
