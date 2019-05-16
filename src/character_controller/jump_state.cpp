#include "character_controller/jump_state.hpp"
#include "character_controller/movement.hpp"
#include "character_controller/stand_state.hpp"
#include "util/settings.hpp"
#include <btBulletDynamicsCommon.h>
#include <spdlog/spdlog.h>
#include <cmath>
#include <optional>

namespace oo {

std::optional<StandState>
JumpState::update(CharacterMediator &mediator, float elapsed) {
  // Apply gravity
  // Apply the spring force if player is sufficiently near to the ground
  // Return the stand state when sufficiently near to the ground.
  mediator.updateCameraOrientation();

  return std::nullopt;
}

void JumpState::enter(CharacterMediator &mediator) {
  mediator.setSpeedModifier(oo::makeSpeedModifier(mediator));
  // Apply a gravitational impulse
  // m * sqrt(2 * g * jumpHeight(Acrobatics))
  // negative to gravity direction to get the right jump height, or otherwise.
}

//std::optional<StandState>
//JumpState::handleCollision(CharacterMediator &mediator,
//                           const btCollisionObject *,
//                           const btManifoldPoint &contact) {
//  const auto impulse{contact.getAppliedImpulse()};
//  const auto r{contact.getPositionWorldOnA() - contact.getPositionWorldOnB()};
//  const auto gravityVector{mediator.getRigidBody()->getGravity()};
//  spdlog::get(oo::LOG)->info("Player received of impulse {} N", impulse);
//  if (r.normalized().dot(gravityVector.normalized()) > std::sqrt(2.0f) / 2.0f) {
//    return StandState{};
//  }
//  return std::nullopt;
//}

} // namespace oo