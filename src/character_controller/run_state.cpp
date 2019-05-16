#include "character_controller/run_state.hpp"
#include "character_controller/walk_state.hpp"

namespace oo {

std::optional<WalkState>
RunState::handleEvent(CharacterMediator &, const event::Run &) {
  return std::make_optional<WalkState>();
}

std::optional<WalkState>
RunState::handleEvent(CharacterMediator &, const event::AlwaysRun &event) {
  return event.down ? std::make_optional<WalkState>() : std::nullopt;
}

std::optional<RunState>
RunState::update(CharacterMediator &, float) {
  return std::nullopt;
}

void RunState::enter(CharacterMediator &mediator) {
  mediator.setIsRunning(true);
}

} // namespace oo
