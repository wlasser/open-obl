#include "engine/player_controller/player_controller_impl.hpp"
#include "engine/player_controller/player_jump_state.hpp"
#include "engine/player_controller/player_sneak_stand_state.hpp"
#include "engine/player_controller/player_stand_state.hpp"
#include "engine/settings.hpp"
#include <spdlog/spdlog.h>

namespace engine {

std::optional<PlayerJumpState>
PlayerStandState::handleEvent(PlayerControllerImpl &impl,
                              const event::Jump &event) {
  return event.down ? std::make_optional<PlayerJumpState>() : std::nullopt;
}

std::optional<PlayerSneakStandState>
PlayerStandState::handleEvent(PlayerControllerImpl &impl,
                              const event::Sneak &event) {
  return event.down ? std::make_optional<PlayerSneakStandState>()
                    : std::nullopt;
}

std::optional<PlayerStandState>
PlayerStandState::update(PlayerControllerImpl &impl, float elapsed) {
  impl.updatePhysics(elapsed);
  return std::nullopt;
}

} // namespace engine
