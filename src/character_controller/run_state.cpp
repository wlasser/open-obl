#include "character_controller/run_state.hpp"
#include "character_controller/walk_state.hpp"

namespace oo {

std::optional<WalkState> RunState::handleEvent(CharacterControllerImpl &/*impl*/,
                                               const event::Run &/*event*/) {
  return std::make_optional<WalkState>();
}

std::optional<WalkState> RunState::handleEvent(CharacterControllerImpl &/*impl*/,
                                               const event::AlwaysRun &event) {
  return event.down ? std::make_optional<WalkState>() : std::nullopt;
}

std::optional<RunState>
RunState::update(CharacterControllerImpl &/*impl*/, float /*elapsed*/) {
  return std::nullopt;
}

void RunState::enter(CharacterControllerImpl &impl) {
  impl.setIsRunning(true);
}

} // namespace oo
