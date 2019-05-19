#include "character_controller/movement.hpp"
#include "character_controller/sneak_jump_state.hpp"
#include "character_controller/sneak_stand_state.hpp"
#include "character_controller/stand_state.hpp"

namespace oo {

std::optional<SneakJumpState>
SneakStandState::handleEvent(CharacterMediator &, const event::Jump &event) {
  return event.down ? std::make_optional<SneakJumpState>() : std::nullopt;
}

std::optional<StandState>
SneakStandState::handleEvent(CharacterMediator &, const event::Sneak &event) {
  return event.down ? std::make_optional<StandState>() : std::nullopt;
}

std::optional<SneakStandState>
SneakStandState::update(CharacterMediator &mediator, float elapsed) {
  // Apply spring force
  mediator.updateCamera();

  return std::nullopt;
}

void SneakStandState::enter(CharacterMediator &mediator) {
  // Set camera position?
  mediator.setSpeedModifier(oo::makeSpeedModifier(mediator));
}

void SneakStandState::exit(CharacterMediator &mediator) {
  // Set camera position back?
}

} // namespace oo
