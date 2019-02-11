#ifndef OPENOBLIVION_CHARACTER_CONTROLLER_PLAYER_CONTROLLER_HPP
#define OPENOBLIVION_CHARACTER_CONTROLLER_PLAYER_CONTROLLER_HPP

#include "controls.hpp"
#include "conversions.hpp"
#include "character_controller/player_controller_impl.hpp"
#include "character_controller/fallback_state.hpp"
#include "character_controller/jump_state.hpp"
#include "character_controller/run_state.hpp"
#include "character_controller/sneak_jump_state.hpp"
#include "character_controller/sneak_stand_state.hpp"
#include "character_controller/stand_state.hpp"
#include "character_controller/walk_state.hpp"
#include "ogrebullet/motion_state.hpp"
#include "game_settings.hpp"
#include <btBulletDynamicsCommon.h>
#include <gsl/gsl>
#include <Ogre.h>
#include <variant>

namespace oo {

using StateVariant = std::variant<StandState,
                                  JumpState,
                                  SneakStandState,
                                  SneakJumpState>;
using MovementStateVariant = std::variant<WalkState, RunState>;

class PlayerController {
 public:
  explicit PlayerController(Ogre::SceneManager *scnMgr,
                            btDiscreteDynamicsWorld *world);

  Ogre::Camera *getCamera() const noexcept;
  btRigidBody *getRigidBody() const noexcept;

  void handleEvent(const KeyVariant &event);
  void handleEvent(const MouseVariant &event);

  void update(float elapsed);

  void moveTo(const Ogre::Vector3 &position);

  void handleCollision(const btCollisionObject *other,
                       const btManifoldPoint &contact);

 private:
  PlayerControllerImpl mImpl{};
  StateVariant mState{};
  MovementStateVariant mMovementStateVariant{};

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

} // namespace oo

#endif // OPENOBLIVION_CHARACTER_CONTROLLER_PLAYER_CONTROLLER_HPP
