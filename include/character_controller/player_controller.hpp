#ifndef OPENOBLIVION_CHARACTER_CONTROLLER_PLAYER_CONTROLLER_HPP
#define OPENOBLIVION_CHARACTER_CONTROLLER_PLAYER_CONTROLLER_HPP

#include "controls.hpp"
#include "character_controller/player_controller_impl.hpp"
#include "character_controller/fallback_state.hpp"
#include "character_controller/jump_state.hpp"
#include "character_controller/run_state.hpp"
#include "character_controller/sneak_jump_state.hpp"
#include "character_controller/sneak_stand_state.hpp"
#include "character_controller/stand_state.hpp"
#include "character_controller/walk_state.hpp"
#include "game_settings.hpp"
#include "math/conversions.hpp"
#include "ogrebullet/motion_state.hpp"
#include <btBulletDynamicsCommon.h>
#include <gsl/gsl>
#include <variant>

namespace oo {

using StateVariant = std::variant<StandState,
                                  JumpState,
                                  SneakStandState,
                                  SneakJumpState>;
using MovementStateVariant = std::variant<WalkState, RunState>;

class PlayerController {
 public:
  explicit PlayerController(gsl::not_null<Ogre::SceneManager *> scnMgr,
                            gsl::not_null<btDiscreteDynamicsWorld *> world);

  Ogre::Camera *getCamera() const noexcept;
  btRigidBody *getRigidBody() const noexcept;

  void handleEvent(const KeyVariant &event);
  void handleEvent(const MouseVariant &event);

  void update(float elapsed);

  void moveTo(const Ogre::Vector3 &position);
  void setOrientation(const Ogre::Quaternion &orientation);

  void handleCollision(const btCollisionObject *other,
                       const btManifoldPoint &contact);

  Ogre::Vector3 getPosition() const noexcept;

 private:
  PlayerControllerImpl mImpl;
  StateVariant mState{};
  MovementStateVariant mMovementStateVariant{};

  void enter(StateVariant &state);
  void enter(MovementStateVariant &state);
  void exit(StateVariant &state);
  void exit(MovementStateVariant &state);
  void changeState(StateVariant newState);
  void changeState(MovementStateVariant newState);
};

} // namespace oo

#endif // OPENOBLIVION_CHARACTER_CONTROLLER_PLAYER_CONTROLLER_HPP
