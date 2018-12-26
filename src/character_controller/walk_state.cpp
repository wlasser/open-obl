#include "character_controller/run_state.hpp"
#include "character_controller/walk_state.hpp"

namespace oo {

std::optional<RunState> WalkState::handleEvent(PlayerControllerImpl &/*impl*/,
                                               const event::Run &/*event*/) {
  return std::make_optional<RunState>();
}

std::optional<RunState> WalkState::handleEvent(PlayerControllerImpl &/*impl*/,
                                               const event::AlwaysRun &event) {
  return event.down ? std::make_optional<RunState>() : std::nullopt;
}

std::optional<WalkState>
WalkState::update(PlayerControllerImpl &/*impl*/, float /*elapsed*/) {
  return std::nullopt;
}

void WalkState::enter(PlayerControllerImpl &impl) {
  impl.isRunning = false;
}

} // namespace oo
