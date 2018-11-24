#ifndef OPENOBLIVION_CHARACTER_CONTROLLER_RUN_STATE_HPP
#define OPENOBLIVION_CHARACTER_CONTROLLER_RUN_STATE_HPP

#include "character_controller/abilities.hpp"
#include "character_controller/fallback_state.hpp"
#include "character_controller/player_controller_impl.hpp"
#include <optional>

namespace oo {

class WalkState;

class RunState : public FallbackState<RunState>,
                 public CollideAbility<RunState> {
 public:
  using FallbackState::handleEvent;
  using CollideAbility::handleCollision;

  std::optional<WalkState>
  handleEvent(PlayerControllerImpl &impl, const event::Run &event);

  std::optional<WalkState>
  handleEvent(PlayerControllerImpl &impl, const event::AlwaysRun &event);

  std::optional<RunState>
  update(PlayerControllerImpl &impl, float elapsed);

  void enter(PlayerControllerImpl &impl);
  void exit(PlayerControllerImpl &) {}
};

} // namespace oo

#endif // OPENOBLIVION_CHARACTER_CONTROLLER_RUN_STATE_HPP
