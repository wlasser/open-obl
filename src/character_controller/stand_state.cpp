#include "character_controller/movement.hpp"
#include "character_controller/jump_state.hpp"
#include "character_controller/sneak_stand_state.hpp"
#include "character_controller/stand_state.hpp"

namespace oo {

std::optional<JumpState>
StandState::handleEvent(CharacterMediator &, const event::Jump &event) {
  return event.down ? std::make_optional<JumpState>() : std::nullopt;
}

std::optional<SneakStandState>
StandState::handleEvent(CharacterMediator &, const event::Sneak &event) {
  return event.down ? std::make_optional<SneakStandState>() : std::nullopt;
}

std::optional<StandState>
StandState::update(CharacterMediator &mediator, float elapsed) {
  // Apply spring force
  mediator.updateCameraOrientation();

  return std::nullopt;
}

void StandState::enter(CharacterMediator &mediator) {
  mediator.setSpeedModifier(oo::makeSpeedModifier(mediator));
}

} // namespace oo
