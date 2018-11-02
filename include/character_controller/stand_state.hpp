#ifndef OPENOBLIVION_CHARACTER_CONTROLLER_STAND_STATE_HPP
#define OPENOBLIVION_CHARACTER_CONTROLLER_STAND_STATE_HPP

#include "character_controller/abilities.hpp"
#include "character_controller/fallback_state.hpp"
#include "character_controller/player_controller_impl.hpp"
#include <optional>
#include <variant>

namespace character {

class JumpState;
class SneakStandState;

class StandState : public FallbackState<StandState>,
                   public MoveAbility<StandState>,
                   public LookAbility<StandState>,
                   public CollideAbility<StandState> {
 public:
  using MoveAbility::handleEvent;
  using LookAbility::handleEvent;
  using CollideAbility::handleCollision;
  using FallbackState::handleEvent;

  std::optional<JumpState>
  handleEvent(PlayerControllerImpl &impl, const event::Jump &event);

  std::optional<SneakStandState>
  handleEvent(PlayerControllerImpl &impl, const event::Sneak &event);

  std::optional<StandState>
  update(PlayerControllerImpl &impl, float elapsed);

  void enter(PlayerControllerImpl &impl);
  void exit(PlayerControllerImpl &) {}
};

} // namespace character

#endif // OPENOBLIVION_CHARACTER_CONTROLLER_STAND_STATE_HPP
