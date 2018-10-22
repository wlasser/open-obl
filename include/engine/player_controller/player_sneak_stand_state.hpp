#ifndef OPENOBLIVION_ENGINE_PLAYER_SNEAK_STAND_STATE_HPP
#define OPENOBLIVION_ENGINE_PLAYER_SNEAK_STAND_STATE_HPP

#include "engine/player_controller/abilities.hpp"
#include "engine/player_controller/player_controller_impl.hpp"
#include "engine/player_controller/player_state.hpp"
#include <optional>
#include <memory>

namespace engine {

class PlayerSneakJumpState;
class PlayerStandState;

class PlayerSneakStandState : public PlayerState<PlayerSneakStandState>,
                              public MoveAbility<PlayerSneakStandState>,
                              public LookAbility<PlayerSneakStandState> {
 public:
  using MoveAbility::handleEvent;
  using LookAbility::handleEvent;
  using PlayerState::handleEvent;
  using PlayerState::handleCollision;

  std::optional<PlayerSneakJumpState>
  handleEvent(PlayerControllerImpl &impl, const event::Jump &event);

  std::optional<PlayerStandState>
  handleEvent(PlayerControllerImpl &impl, const event::Sneak &event);

  std::optional<PlayerSneakStandState>
  update(PlayerControllerImpl &impl, float elapsed);

  void enter(PlayerControllerImpl &impl);
  void exit(PlayerControllerImpl &impl);
};

} // namespace engine

#endif // OPENOBLIVION_ENGINE_PLAYER_SNEAK_STAND_STATE_HPP
