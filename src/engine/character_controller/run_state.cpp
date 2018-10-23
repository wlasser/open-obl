#include "engine/character_controller/run_state.hpp"
#include "engine/character_controller/walk_state.hpp"
#include "engine/settings.hpp"
#include <spdlog/spdlog.h>

namespace engine::character {

std::optional<WalkState>
RunState::handleEvent(PlayerControllerImpl &impl, const event::Run &event) {
  return std::make_optional<WalkState>();
}

std::optional<WalkState>
RunState::handleEvent(PlayerControllerImpl &impl,
                      const event::AlwaysRun &event) {
  return event.down ? std::make_optional<WalkState>() : std::nullopt;
}

std::optional<RunState>
RunState::update(PlayerControllerImpl &impl, float elapsed) {
  return std::nullopt;
}

void RunState::enter(PlayerControllerImpl &impl) {
  impl.isRunning = true;
  spdlog::get(settings::log)->info("Entered RunState");
}

} // namespace engine::character
