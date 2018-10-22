#include "engine/character_controller/player_controller_impl.hpp"
#include "engine/character_controller/sneak_jump_state.hpp"
#include "engine/character_controller/sneak_stand_state.hpp"
#include "engine/settings.hpp"
#include <btBulletDynamicsCommon.h>
#include <spdlog/spdlog.h>
#include <cmath>
#include <memory>

namespace engine::character {

std::optional<SneakJumpState>
SneakJumpState::update(PlayerControllerImpl &impl, float elapsed) {
  impl.updatePhysics(elapsed);
  return std::nullopt;
}

void SneakJumpState::enter(PlayerControllerImpl &impl) {
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

} // namespace engine::character
