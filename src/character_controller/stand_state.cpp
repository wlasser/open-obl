#include "character_controller/character_controller_impl.hpp"
#include "character_controller/movement.hpp"
#include "character_controller/jump_state.hpp"
#include "character_controller/sneak_stand_state.hpp"
#include "character_controller/stand_state.hpp"

namespace oo {

std::optional<JumpState>
StandState::handleEvent(CharacterControllerImpl &/*impl*/,
                        const event::Jump &event) {
  return event.down ? std::make_optional<JumpState>() : std::nullopt;
}

std::optional<SneakStandState>
StandState::handleEvent(CharacterControllerImpl &/*impl*/,
                        const event::Sneak &event) {
  return event.down ? std::make_optional<SneakStandState>()
                    : std::nullopt;
}

std::optional<StandState>
StandState::update(CharacterControllerImpl &impl, float elapsed) {
  impl.updatePhysics(elapsed);
  impl.applySpringForce(impl.getSpringDisplacement());
  return std::nullopt;
}

void StandState::enter(CharacterControllerImpl &impl) {
  impl.setSpeedModifier([&impl](bool hasWeaponOut, bool isRunning) {
    return (isRunning ? oo::runModifier(impl.athleticsSkill) : 1.0f)
        * oo::weaponOutModifier(hasWeaponOut);
  });
}

} // namespace oo
