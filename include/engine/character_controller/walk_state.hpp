#ifndef OPENOBLIVION_ENGINE_CHARACTER_CONTROLLER_WALK_STATE_HPP
#define OPENOBLIVION_ENGINE_CHARACTER_CONTROLLER_WALK_STATE_HPP

#include "engine/character_controller/fallback_state.hpp"
#include "engine/character_controller/player_controller_impl.hpp"
#include <optional>

namespace engine::character {

class RunState;

class WalkState : public FallbackState<WalkState> {
 public:
  using FallbackState::handleEvent;
  using FallbackState::exit;
  using FallbackState::handleCollision;

  std::optional<RunState>
  handleEvent(PlayerControllerImpl &impl, const event::Run &event);

  std::optional<RunState>
  handleEvent(PlayerControllerImpl &impl, const event::AlwaysRun &event);

  std::optional<WalkState>
  update(PlayerControllerImpl &impl, float elapsed);

  void enter(PlayerControllerImpl &impl);
};

} // namespace engine::character

#endif // OPENOBLIVION_ENGINE_CHARACTER_CONTROLLER_WALK_STATE_HPP
