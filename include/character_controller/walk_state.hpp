#ifndef OPENOBL_CHARACTER_CONTROLLER_WALK_STATE_HPP
#define OPENOBL_CHARACTER_CONTROLLER_WALK_STATE_HPP

#include "character_controller/abilities.hpp"
#include "character_controller/character_mediator.hpp"
#include "character_controller/fallback_state.hpp"
#include <optional>

namespace oo {

class RunState;

class WalkState : public FallbackState<WalkState>,
                  public CollideAbility<WalkState> {
 public:
  using FallbackState::handleEvent;
  using CollideAbility::handleCollision;

  std::optional<RunState>
  handleEvent(CharacterMediator &mediator, const event::Run &event);

  std::optional<RunState>
  handleEvent(CharacterMediator &mediator, const event::AlwaysRun &event);

  std::optional<WalkState> update(CharacterMediator &mediator, float elapsed);

  void enter(CharacterMediator &mediator);
  void exit(CharacterMediator &) {}
};

} // namespace oo

#endif // OPENOBL_CHARACTER_CONTROLLER_WALK_STATE_HPP
