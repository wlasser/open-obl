#include "character_controller/player_controller_impl.hpp"
#include "character_controller/jump_state.hpp"
#include "character_controller/movement.hpp"
#include "character_controller/stand_state.hpp"
#include "settings.hpp"
#include <btBulletDynamicsCommon.h>
#include <spdlog/spdlog.h>
#include <cmath>
#include <optional>

namespace oo {

std::optional<StandState>
JumpState::update(PlayerControllerImpl &impl, float elapsed) {
  impl.updatePhysics(elapsed);

  // Only apply the spring force if the player is falling, and sufficiently near
  // to the ground.
  const btVector3 gravityVector{impl.getRigidBody()->getGravity()};
  const btVector3 v{impl.getRigidBody()->getLinearVelocity()};
  if (v.dot(gravityVector) > 0.0f) {
    const auto displacement{impl.getSpringDisplacement()};
    // As the player is falling, displacement is negative, getting closer to
    // zero. Once displacement is sufficiently small, the player has landed.
    if (std::abs(displacement) < 0.1f) {
      return StandState{};
    } else if (displacement > -impl.getMaxSpringDisplacement()) {
      impl.applySpringForce(displacement);
    }
  }

  return std::nullopt;
}

void JumpState::enter(PlayerControllerImpl &impl) {
  impl.setSpeedModifier([&impl](bool hasWeaponOut, bool isRunning) {
    return (isRunning ? oo::runModifier(impl.athleticsSkill) : 1.0f)
        * oo::weaponOutModifier(hasWeaponOut);
  });
  // Player jumps in the opposite direction of gravity, with an impulse chosen
  // to give the desired jump height. To find the impulse, use v^2 = u^2 + 2as
  // along with the fact that the impulse is the change in momentum.
  // TODO: The impulse should be calculated for some baseline gravity that the
  //       player is 'used to', so that if the player goes somewhere with a
  //       different amount of gravity then their jump height will change.
  const btVector3 gravityVector{impl.getRigidBody()->getGravity()};
  const float g{gravityVector.length()};
  const float apex{oo::jumpHeight(impl.acrobaticsSkill)};
  const float impulse{impl.mass * std::sqrt(2.0f * g * apex)};
  impl.getRigidBody()->applyCentralImpulse(
      -impulse * gravityVector.normalized());
}

std::optional<StandState>
JumpState::handleCollision(PlayerControllerImpl &impl,
                           const btCollisionObject *,
                           const btManifoldPoint &contact) {
  const auto impulse{contact.getAppliedImpulse()};
  const auto r{contact.getPositionWorldOnA() - contact.getPositionWorldOnB()};
  const auto gravityVector{impl.getRigidBody()->getGravity()};
  spdlog::get(oo::LOG)->info("Player received of impulse {} N", impulse);
  if (r.normalized().dot(gravityVector.normalized()) > std::sqrt(2.0f) / 2.0f) {
    return StandState{};
  }
  return std::nullopt;
}

} // namespace oo