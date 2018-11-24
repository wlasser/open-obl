#ifndef OPENOBLIVION_CHARACTER_CONTROLLER_WALK_STATE_HPP
#define OPENOBLIVION_CHARACTER_CONTROLLER_WALK_STATE_HPP

#include "character_controller/abilities.hpp"
#include "character_controller/fallback_state.hpp"
#include "character_controller/player_controller_impl.hpp"
#include <optional>

namespace oo {

class RunState;

class WalkState : public FallbackState<WalkState>,
                  public CollideAbility<WalkState> {
 public:
  using FallbackState::handleEvent;
  using CollideAbility::handleCollision;

  std::optional<RunState>
  handleEvent(PlayerControllerImpl &impl, const event::Run &event);

  std::optional<RunState>
  handleEvent(PlayerControllerImpl &impl, const event::AlwaysRun &event);

  std::optional<WalkState>
  update(PlayerControllerImpl &impl, float elapsed);

  void enter(PlayerControllerImpl &impl);
  void exit(PlayerControllerImpl &) {}
};

} // namespace oo

#endif // OPENOBLIVION_CHARACTER_CONTROLLER_WALK_STATE_HPP
