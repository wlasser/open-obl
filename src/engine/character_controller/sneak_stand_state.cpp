#include "engine/character_controller/player_controller_impl.hpp"
#include "engine/character_controller/sneak_jump_state.hpp"
#include "engine/character_controller/sneak_stand_state.hpp"
#include "engine/character_controller/stand_state.hpp"
#include "engine/settings.hpp"
#include <spdlog/spdlog.h>

namespace engine::character {

std::optional<SneakJumpState>
SneakStandState::handleEvent(PlayerControllerImpl &impl,
                             const event::Jump &event) {
  return event.down ? std::make_optional<SneakJumpState>() : std::nullopt;
}

std::optional<StandState>
SneakStandState::handleEvent(PlayerControllerImpl &impl,
                             const event::Sneak &event) {
  return event.down ? std::make_optional<StandState>() : std::nullopt;
}

std::optional<SneakStandState>
SneakStandState::update(PlayerControllerImpl &impl, float elapsed) {
  impl.updatePhysics(elapsed);
  return std::nullopt;
}

void SneakStandState::enter(PlayerControllerImpl &impl) {
  impl.cameraNode->setPosition({0.0f, impl.height * 0.25f, 0.0f});
}

void SneakStandState::exit(PlayerControllerImpl &impl) {
  impl.cameraNode->setPosition({0.01, impl.height * 0.45f, 0.0f});
}

} // namespace engine::character
