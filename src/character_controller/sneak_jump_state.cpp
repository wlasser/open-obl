#include "character_controller/player_controller_impl.hpp"
#include "character_controller/sneak_jump_state.hpp"
#include "character_controller/sneak_stand_state.hpp"
#include "settings.hpp"
#include <btBulletDynamicsCommon.h>
#include <spdlog/spdlog.h>
#include <cmath>
#include <memory>

namespace character {

std::optional<SneakJumpState>
SneakJumpState::update(PlayerControllerImpl &impl, float elapsed) {
  impl.updatePhysics(elapsed);
  return std::nullopt;
}

void SneakJumpState::enter(PlayerControllerImpl &impl) {
  impl.speedModifier = [&impl](bool hasWeaponOut, bool isRunning) {
    return (isRunning ? impl.runModifier(impl.athleticsSkill) : 1.0f)
        * impl.weaponOutModifier(hasWeaponOut) * impl.sneakModifier();
  };
  // Player jumps in the opposite direction of gravity, with an impulse chosen
  // to give the desired jump height. To find the impulse, use v^2 = u^2 + 2as
  // along with the fact that the impulse is the change in momentum.
  const btVector3 gravityVector{impl.rigidBody->getGravity()};
  const float g{gravityVector.length()};
  const float apex{impl.jumpHeight(impl.acrobaticsSkill)};
  const float impulse{impl.mass * std::sqrt(2.0f * g * apex)};
  impl.rigidBody->applyCentralImpulse(-impulse * gravityVector.normalized());
}

std::optional<SneakStandState>
SneakJumpState::handleCollision(PlayerControllerImpl &impl,
                                const btCollisionObject *other,
                                const btManifoldPoint &contact) {
  const auto impulse{contact.getAppliedImpulse()};
  const auto r{contact.getPositionWorldOnA() - contact.getPositionWorldOnB()};
  spdlog::get(settings::log)->info("Player received of impulse {} N", impulse);
  if (r.normalized().dot(impl.rigidBody->getGravity().normalized()) > 0.7) {
    return SneakStandState{};
  }
  return std::nullopt;
}

} // namespace character
