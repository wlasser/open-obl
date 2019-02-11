#include "character_controller/player_controller_impl.hpp"
#include "character_controller/sneak_jump_state.hpp"
#include "character_controller/sneak_stand_state.hpp"
#include "settings.hpp"
#include <btBulletDynamicsCommon.h>
#include <spdlog/spdlog.h>
#include <cmath>
#include <memory>

namespace oo {

std::optional<SneakStandState>
SneakJumpState::update(PlayerControllerImpl &impl, float elapsed) {
  impl.updatePhysics(elapsed);

  // Only apply the spring force if the player is falling, and sufficiently near
  // to the ground.
  const btVector3 gravityVector{impl.rigidBody->getGravity()};
  const btVector3 v{impl.rigidBody->getLinearVelocity()};
  if (v.dot(gravityVector) > 0.0f) {
    const auto displacement{impl.getSpringDisplacement()};
    // As the player is falling, displacement is negative, getting closer to
    // zero. Once displacement is sufficiently small, the player has landed.
    if (std::abs(displacement) < 0.1f) {
      return SneakStandState{};
    } else if (displacement > -impl.getMaxSpringDisplacement()) {
      impl.applySpringForce(displacement);
    }
  }

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
                                const btCollisionObject */*other*/,
                                const btManifoldPoint &contact) {
  const auto impulse{contact.getAppliedImpulse()};
  const auto r{contact.getPositionWorldOnA() - contact.getPositionWorldOnB()};
  spdlog::get(oo::LOG)->info("Player received of impulse {} N", impulse);
  if (r.normalized().dot(impl.rigidBody->getGravity().normalized()) > 0.7) {
    return SneakStandState{};
  }
  return std::nullopt;
}

} // namespace oo
