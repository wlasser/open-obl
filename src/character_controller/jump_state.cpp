#include "character_controller/player_controller_impl.hpp"
#include "character_controller/jump_state.hpp"
#include "character_controller/stand_state.hpp"
#include "settings.hpp"
#include <btBulletDynamicsCommon.h>
#include <spdlog/spdlog.h>
#include <cmath>
#include <optional>

namespace oo {

std::optional<JumpState>
JumpState::update(PlayerControllerImpl &impl, float elapsed) {
  impl.updatePhysics(elapsed);
  return std::nullopt;
}

void JumpState::enter(PlayerControllerImpl &impl) {
  impl.speedModifier = [&impl](bool hasWeaponOut, bool isRunning) {
    return (isRunning ? impl.runModifier(impl.athleticsSkill) : 1.0f)
        * impl.weaponOutModifier(hasWeaponOut);
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

std::optional<StandState>
JumpState::handleCollision(PlayerControllerImpl &impl,
                           const btCollisionObject *,
                           const btManifoldPoint &contact) {
  const auto impulse{contact.getAppliedImpulse()};
  const auto r{contact.getPositionWorldOnA() - contact.getPositionWorldOnB()};
  const auto gravityVector{impl.rigidBody->getGravity()};
  spdlog::get(oo::LOG)->info("Player received of impulse {} N", impulse);
  if (r.normalized().dot(gravityVector.normalized()) > std::sqrt(2.0f) / 2.0f) {
    return StandState{};
  }
  return std::nullopt;
}

} // namespace oo