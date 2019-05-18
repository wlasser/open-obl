#include "character_controller/movement.hpp"
#include "character_controller/jump_state.hpp"
#include "character_controller/sneak_stand_state.hpp"
#include "character_controller/stand_state.hpp"
#include "settings.hpp"
#include <spdlog/spdlog.h>

namespace oo {

std::optional<JumpState>
StandState::handleEvent(CharacterMediator &, const event::Jump &event) {
  return event.down ? std::make_optional<JumpState>() : std::nullopt;
}

std::optional<SneakStandState>
StandState::handleEvent(CharacterMediator &, const event::Sneak &event) {
  return event.down ? std::make_optional<SneakStandState>() : std::nullopt;
}

std::optional<JumpState>
StandState::update(CharacterMediator &mediator, float elapsed) {
  // Apply spring force
  mediator.updateCameraOrientation();

  const auto result{mediator.raycast()};
  auto dist{qvm::mag(result.m_hitPointWorld - result.m_rayFromWorld)};
  dist -= mediator.getHeight() * 0.5f;
  if (dist > 0.1f || result.m_hasHit) {
    return std::make_optional<JumpState>();
  }

  return std::nullopt;
}

void StandState::enter(CharacterMediator &mediator) {
  mediator.setSpeedModifier(oo::makeSpeedModifier(mediator));
  qvm::Y(mediator.getLocalVelocity()) = 0.0f;
}

} // namespace oo
