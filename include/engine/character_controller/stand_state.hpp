#ifndef OPENOBLIVION_ENGINE_CHARACTER_CONTROLLER_STAND_STATE_HPP
#define OPENOBLIVION_ENGINE_CHARACTER_CONTROLLER_STAND_STATE_HPP

#include "engine/character_controller/abilities.hpp"
#include "engine/character_controller/fallback_state.hpp"
#include "engine/character_controller/player_controller_impl.hpp"
#include <optional>
#include <variant>

namespace engine::character {

class JumpState;
class SneakStandState;

class StandState : public FallbackState<StandState>,
                   public MoveAbility<StandState>,
                   public LookAbility<StandState> {
 public:
  using MoveAbility::handleEvent;
  using LookAbility::handleEvent;
  using FallbackState::handleEvent;
  using FallbackState::handleCollision;
  using FallbackState::enter;
  using FallbackState::exit;

  std::optional<JumpState>
  handleEvent(PlayerControllerImpl &impl, const event::Jump &event);

  std::optional<SneakStandState>
  handleEvent(PlayerControllerImpl &impl, const event::Sneak &event);

  std::optional<StandState>
  update(PlayerControllerImpl &impl, float elapsed);
};

} // namespace engine::character

#endif // OPENOBLIVION_ENGINE_CHARACTER_CONTROLLER_STAND_STATE_HPP
