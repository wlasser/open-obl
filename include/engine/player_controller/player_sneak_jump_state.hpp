#ifndef OPENOBLIVION_ENGINE_PLAYER_SNEAK_JUMP_STATE_HPP
#define OPENOBLIVION_ENGINE_PLAYER_SNEAK_JUMP_STATE_HPP

#include "engine/player_controller/abilities.hpp"
#include "engine/player_controller/player_controller_impl.hpp"
#include "engine/player_controller/player_state.hpp"
#include <optional>
#include <memory>

namespace engine {

class PlayerSneakStandState;
class PlayerStandState;

class PlayerSneakJumpState : public PlayerState<PlayerSneakJumpState>,
                             public MoveAbility<PlayerSneakJumpState>,
                             public LookAbility<PlayerSneakJumpState> {
 public:
  using MoveAbility::handleEvent;
  using LookAbility::handleEvent;
  using PlayerState::handleEvent;
  using PlayerState::exit;

  std::optional<PlayerSneakJumpState>
  update(PlayerControllerImpl &impl, float elapsed);

  std::optional<PlayerSneakStandState>
  handleCollision(PlayerControllerImpl &impl,
                  const btCollisionObject *other,
                  const btManifoldPoint &contact);

  void enter(PlayerControllerImpl &impl);
};

} // namespace engine

#endif // OPENOBLIVION_ENGINE_PLAYER_SNEAK_JUMP_STATE_HPP
