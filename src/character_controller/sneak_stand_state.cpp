#include "character_controller/player_controller_impl.hpp"
#include "character_controller/sneak_jump_state.hpp"
#include "character_controller/sneak_stand_state.hpp"
#include "character_controller/stand_state.hpp"

namespace oo {

std::optional<SneakJumpState>
SneakStandState::handleEvent(PlayerControllerImpl &/*impl*/,
                             const event::Jump &event) {
  return event.down ? std::make_optional<SneakJumpState>() : std::nullopt;
}

std::optional<StandState>
SneakStandState::handleEvent(PlayerControllerImpl &/*impl*/,
                             const event::Sneak &event) {
  return event.down ? std::make_optional<StandState>() : std::nullopt;
}

std::optional<SneakStandState>
SneakStandState::update(PlayerControllerImpl &impl, float elapsed) {
  impl.updatePhysics(elapsed);
  impl.applySpringForce(impl.getSpringDisplacement());
  return std::nullopt;
}

void SneakStandState::enter(PlayerControllerImpl &impl) {
  impl.cameraNode->setPosition(Ogre::Vector3::ZERO);
  impl.speedModifier = [&impl](bool hasWeaponOut, bool isRunning) {
    return (isRunning ? impl.runModifier(impl.athleticsSkill) : 1.0f)
        * impl.weaponOutModifier(hasWeaponOut) * impl.sneakModifier();
  };
}

void SneakStandState::exit(PlayerControllerImpl &impl) {
  const auto h{(0.95f - 0.5f) * impl.height - impl.getCapsuleHeight() / 2.0f};
  const auto camVector{qvm::convert_to<Ogre::Vector3>(qvm::_0X0(h))};
  impl.cameraNode->setPosition(camVector);
}

} // namespace oo
