#include "character_controller/character_controller_impl.hpp"
#include "character_controller/movement.hpp"
#include "character_controller/sneak_jump_state.hpp"
#include "character_controller/sneak_stand_state.hpp"
#include "character_controller/stand_state.hpp"

namespace oo {

std::optional<SneakJumpState>
SneakStandState::handleEvent(CharacterControllerImpl &/*impl*/,
                             const event::Jump &event) {
  return event.down ? std::make_optional<SneakJumpState>() : std::nullopt;
}

std::optional<StandState>
SneakStandState::handleEvent(CharacterControllerImpl &/*impl*/,
                             const event::Sneak &event) {
  return event.down ? std::make_optional<StandState>() : std::nullopt;
}

std::optional<SneakStandState>
SneakStandState::update(CharacterControllerImpl &impl, float elapsed) {
  impl.updatePhysics(elapsed);
  impl.applySpringForce(impl.getSpringDisplacement());
  return std::nullopt;
}

void SneakStandState::enter(CharacterControllerImpl &impl) {
  impl.getCameraNode()->setPosition(Ogre::Vector3::ZERO);
  impl.setSpeedModifier([&impl](bool hasWeaponOut, bool isRunning) {
    const auto athleticsSkill{impl.getSkill(SkillIndex::Athletics)};
    return (isRunning ? oo::runModifier(athleticsSkill) : 1.0f)
        * oo::weaponOutModifier(hasWeaponOut) * oo::sneakModifier();
  });
}

void SneakStandState::exit(CharacterControllerImpl &impl) {
  const auto h{(0.95f - 0.5f) * impl.getHeight()
                   - impl.getCapsuleHeight() / 2.0f};
  const auto camVector{qvm::convert_to<Ogre::Vector3>(qvm::_0X0(h))};
  impl.getCameraNode()->setPosition(camVector);
}

} // namespace oo
