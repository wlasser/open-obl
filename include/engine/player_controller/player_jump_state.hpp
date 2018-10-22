#ifndef OPENOBLIVION_ENGINE_PLAYER_JUMP_STATE_HPP
#define OPENOBLIVION_ENGINE_PLAYER_JUMP_STATE_HPP

#include "engine/player_controller/abilities.hpp"
#include "engine/player_controller/player_controller_impl.hpp"
#include "engine/player_controller/player_state.hpp"
#include <memory>
#include <variant>

namespace engine {

class PlayerStandState;

class PlayerJumpState : public PlayerState<PlayerJumpState>,
                        public MoveAbility<PlayerJumpState>,
                        public LookAbility<PlayerJumpState> {
 public:
  using PlayerState::handleEvent;
  using MoveAbility::handleEvent;
  using LookAbility::handleEvent;

  std::optional<PlayerJumpState>
  update(PlayerControllerImpl &player, float elapsed);

  std::optional<PlayerStandState>
  handleCollision(PlayerControllerImpl &impl,
                  const btCollisionObject *other,
                  const btManifoldPoint &contact);

  void enter(PlayerControllerImpl &player);
};

} // namespace engine

#endif // OPENOBLIVION_ENGINE_PLAYER_JUMP_STATE_HPP
