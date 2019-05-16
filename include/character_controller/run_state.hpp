#ifndef OPENOBL_CHARACTER_CONTROLLER_RUN_STATE_HPP
#define OPENOBL_CHARACTER_CONTROLLER_RUN_STATE_HPP

#include "character_controller/abilities.hpp"
#include "character_controller/character_mediator.hpp"
#include "character_controller/fallback_state.hpp"
#include <optional>

namespace oo {

class WalkState;

class RunState : public FallbackState<RunState>,
                 public CollideAbility<RunState> {
 public:
  using FallbackState::handleEvent;
  using CollideAbility::handleCollision;

  std::optional<WalkState>
  handleEvent(CharacterMediator &mediator, const event::Run &event);

  std::optional<WalkState>
  handleEvent(CharacterMediator &mediator, const event::AlwaysRun &event);

  std::optional<RunState> update(CharacterMediator &mediator, float elapsed);

  void enter(CharacterMediator &mediator);
  void exit(CharacterMediator &) {}
};

} // namespace oo

#endif // OPENOBL_CHARACTER_CONTROLLER_RUN_STATE_HPP
