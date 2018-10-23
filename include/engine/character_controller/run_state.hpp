#ifndef OPENOBLIVION_ENGINE_CHARACTER_CONTROLLER_RUN_STATE_HPP
#define OPENOBLIVION_ENGINE_CHARACTER_CONTROLLER_RUN_STATE_HPP

#include "engine/character_controller/fallback_state.hpp"
#include "engine/character_controller/player_controller_impl.hpp"
#include <optional>

namespace engine::character {

class WalkState;

class RunState : public FallbackState<RunState> {
 public:
  using FallbackState::handleEvent;
  using FallbackState::exit;
  using FallbackState::handleCollision;

  std::optional<WalkState>
  handleEvent(PlayerControllerImpl &impl, const event::Run &event);

  std::optional<WalkState>
  handleEvent(PlayerControllerImpl &impl, const event::AlwaysRun &event);

  std::optional<RunState>
  update(PlayerControllerImpl &impl, float elapsed);

  void enter(PlayerControllerImpl &impl);
};

} // namespace engine::character

#endif // OPENOBLIVION_ENGINE_CHARACTER_CONTROLLER_RUN_STATE_HPP
