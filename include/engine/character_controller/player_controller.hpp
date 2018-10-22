#ifndef OPENOBLIVION_ENGINE_CHARACTER_CONTROLLER_PLAYER_CONTROLLER_HPP
#define OPENOBLIVION_ENGINE_CHARACTER_CONTROLLER_PLAYER_CONTROLLER_HPP

#include "engine/controls.hpp"
#include "engine/conversions.hpp"
#include "engine/character_controller/player_controller_impl.hpp"
#include "engine/character_controller/fallback_state.hpp"
#include "engine/character_controller/jump_state.hpp"
#include "engine/character_controller/sneak_jump_state.hpp"
#include "engine/character_controller/sneak_stand_state.hpp"
#include "engine/character_controller/stand_state.hpp"
#include "ogrebullet/motion_state.hpp"
#include "game_settings.hpp"
#include <btBulletDynamicsCommon.h>
#include <Ogre.h>
#include <variant>

namespace engine::character {

template<class B, class A>
std::optional<B> liftOptional(const std::optional<A> &a) {
  return a ? std::optional<B>{*a} : std::nullopt;
}

using StateVariant = std::variant<StandState,
                                  JumpState,
                                  SneakStandState,
                                  SneakJumpState>;

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
};

} // namespace engine::character

#endif // OPENOBLIVION_ENGINE_CHARACTER_CONTROLLER_PLAYER_CONTROLLER_HPP
