#include "engine/player_controller/player_controller_impl.hpp"
#include "engine/player_controller/player_sneak_jump_state.hpp"
#include "engine/player_controller/player_sneak_stand_state.hpp"
#include "engine/player_controller/player_stand_state.hpp"
#include "engine/settings.hpp"
#include <spdlog/spdlog.h>

namespace engine {

std::optional<PlayerSneakJumpState>
PlayerSneakStandState::handleEvent(PlayerControllerImpl &impl,
                                   const event::Jump &event) {
  return event.down ? std::make_optional<PlayerSneakJumpState>() : std::nullopt;
}

std::optional<PlayerStandState>
PlayerSneakStandState::handleEvent(PlayerControllerImpl &impl,
                                   const event::Sneak &event) {
  return event.down ? std::make_optional<PlayerStandState>() : std::nullopt;
}

std::optional<PlayerSneakStandState>
PlayerSneakStandState::update(PlayerControllerImpl &impl, float elapsed) {
  impl.updatePhysics(elapsed);
  return std::nullopt;
}

void PlayerSneakStandState::enter(PlayerControllerImpl &impl) {
  impl.cameraNode->setPosition({0.0f, impl.height * 0.25f, 0.0f});
}

void PlayerSneakStandState::exit(PlayerControllerImpl &impl) {
  impl.cameraNode->setPosition({0.01, impl.height * 0.45f, 0.0f});
}

} // namespace engine
