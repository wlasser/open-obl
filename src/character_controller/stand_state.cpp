#include "character_controller/movement.hpp"
#include "character_controller/jump_state.hpp"
#include "character_controller/sneak_stand_state.hpp"
#include "character_controller/stand_state.hpp"
#include "util/settings.hpp"
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
  mediator.updateCamera();

  const auto &localVelocity{mediator.getLocalVelocity()};
  if (const auto localSpeed{localVelocity.length()}; localSpeed > 0.01f) {
    const auto speed{mediator.getMoveSpeed()};
    const Ogre::Matrix3 frame{mediator.getSurfaceFrame()};
    Ogre::Vector3 velocity{frame * localVelocity / localSpeed * speed};
    velocity.y += localVelocity.y;
    mediator.translate(velocity * elapsed);
  }

  mediator.updateCapsule();

  const auto dist{mediator.getSurfaceDist()};

  if (!dist || dist > 0.1f) {
    return std::make_optional<JumpState>();
  }

  return std::nullopt;
}

void StandState::enter(CharacterMediator &mediator) {
  mediator.setSpeedModifier(oo::makeSpeedModifier(mediator));
  qvm::Y(mediator.getLocalVelocity()) = 0.0f;
}

} // namespace oo
