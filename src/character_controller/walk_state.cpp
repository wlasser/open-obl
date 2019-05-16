#include "character_controller/run_state.hpp"
#include "character_controller/walk_state.hpp"

namespace oo {

std::optional<RunState>
WalkState::handleEvent(CharacterMediator &, const event::Run &) {
  return std::make_optional<RunState>();
}

std::optional<RunState>
WalkState::handleEvent(CharacterMediator &, const event::AlwaysRun &event) {
  return event.down ? std::make_optional<RunState>() : std::nullopt;
}

std::optional<WalkState>
WalkState::update(CharacterMediator &, float) {
  return std::nullopt;
}

void WalkState::enter(CharacterMediator &mediator) {
  mediator.setIsRunning(false);
}

} // namespace oo
