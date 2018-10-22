#include "engine/player_controller/player_controller_impl.hpp"
#include "engine/player_controller/player_jump_state.hpp"
#include "engine/player_controller/player_stand_state.hpp"

namespace engine {

std::optional<PlayerJumpState>
PlayerStandState::handleEvent(PlayerControllerImpl &impl,
                              const event::Jump &event) {
  return PlayerJumpState{};
}

std::optional<PlayerStandState>
PlayerStandState::update(PlayerControllerImpl &impl, float elapsed) {
  impl.updatePhysics(elapsed);
  return std::nullopt;
}

} // namespace engine
