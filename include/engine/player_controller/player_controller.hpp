#ifndef OPENOBLIVION_ENGINE_PLAYER_CONTROLLER_HPP
#define OPENOBLIVION_ENGINE_PLAYER_CONTROLLER_HPP

#include "engine/controls.hpp"
#include "engine/conversions.hpp"
#include "engine/player_controller/player_controller_impl.hpp"
#include "engine/player_controller/player_jump_state.hpp"
#include "engine/player_controller/player_sneak_jump_state.hpp"
#include "engine/player_controller/player_sneak_stand_state.hpp"
#include "engine/player_controller/player_stand_state.hpp"
#include "engine/player_controller/player_state.hpp"
#include "ogrebullet/motion_state.hpp"
#include "game_settings.hpp"
#include <btBulletDynamicsCommon.h>
#include <Ogre.h>
#include <variant>

namespace engine {

template<class B, class A>
std::optional<B> liftOptional(const std::optional<A> &a) {
  return a ? std::optional<B>{*a} : std::nullopt;
}

using PlayerStateVariant = std::variant<PlayerStandState,
                                        PlayerJumpState,
                                        PlayerSneakStandState,
                                        PlayerSneakJumpState>;

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
  PlayerStateVariant state{};
};

} // namespace engine

#endif // OPENOBLIVION_ENGINE_PLAYER_CONTROLLER_HPP
