#ifndef OPENOBLIVION_ENGINE_PLAYER_STAND_STATE_HPP
#define OPENOBLIVION_ENGINE_PLAYER_STAND_STATE_HPP

#include "engine/player_controller/abilities.hpp"
#include "engine/player_controller/player_controller_impl.hpp"
#include "engine/player_controller/player_state.hpp"
#include <optional>
#include <variant>

namespace engine {

class PlayerJumpState;
class PlayerSneakStandState;

class PlayerStandState : public PlayerState<PlayerStandState>,
                         public MoveAbility<PlayerStandState>,
                         public LookAbility<PlayerStandState> {
 public:
  using MoveAbility::handleEvent;
  using LookAbility::handleEvent;
  using PlayerState::handleEvent;
  using PlayerState::handleCollision;
  using PlayerState::enter;
  using PlayerState::exit;

  std::optional<PlayerJumpState>
  handleEvent(PlayerControllerImpl &impl, const event::Jump &event);

  std::optional<PlayerSneakStandState>
  handleEvent(PlayerControllerImpl &impl, const event::Sneak &event);

  std::optional<PlayerStandState>
  update(PlayerControllerImpl &impl, float elapsed);
};

}

#endif // OPENOBLIVION_ENGINE_PLAYER_STAND_STATE_HPP