#include "character_controller/player_controller_impl.hpp"
#include "character_controller/jump_state.hpp"
#include "character_controller/sneak_stand_state.hpp"
#include "character_controller/stand_state.hpp"

namespace oo {

std::optional<JumpState>
StandState::handleEvent(PlayerControllerImpl &/*impl*/,
                        const event::Jump &event) {
  return event.down ? std::make_optional<JumpState>() : std::nullopt;
}

std::optional<SneakStandState>
StandState::handleEvent(PlayerControllerImpl &/*impl*/,
                        const event::Sneak &event) {
  return event.down ? std::make_optional<SneakStandState>()
                    : std::nullopt;
}

std::optional<StandState>
StandState::update(PlayerControllerImpl &impl, float elapsed) {
  impl.updatePhysics(elapsed);
  return std::nullopt;
}

void StandState::enter(PlayerControllerImpl &impl) {
  impl.speedModifier = [&impl](bool hasWeaponOut, bool isRunning) {
    return (isRunning ? impl.runModifier(impl.athleticsSkill) : 1.0f)
        * impl.weaponOutModifier(hasWeaponOut);
  };
}

} // namespace oo
