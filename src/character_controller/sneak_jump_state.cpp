#include "character_controller/movement.hpp"
#include "character_controller/sneak_jump_state.hpp"
#include "character_controller/sneak_stand_state.hpp"
#include "util/settings.hpp"
#include <btBulletDynamicsCommon.h>
#include <spdlog/spdlog.h>
#include <cmath>
#include <memory>

namespace oo {

std::optional<SneakStandState>
SneakJumpState::update(CharacterMediator &mediator, float elapsed) {
  // Apply gravity
  // Apply the spring force if player is sufficiently near to the ground
  // Return the SneakStandState when sufficiently near to the ground.
  mediator.updateCamera();

  return std::nullopt;
}

void SneakJumpState::enter(CharacterMediator &mediator) {
  mediator.setSpeedModifier(oo::makeSneakSpeedModifier(mediator));
  // Apply a gravitational impulse, see JumpState::enter
}

//std::optional<SneakStandState>
//SneakJumpState::handleCollision(CharacterMediator &mediator,
//                                const btCollisionObject */*other*/,
//                                const btManifoldPoint &contact) {
//  const auto impulse{contact.getAppliedImpulse()};
//  const auto r{contact.getPositionWorldOnA() - contact.getPositionWorldOnB()};
//  const auto gravityVector{mediator.getRigidBody()->getGravity()};
//  spdlog::get(oo::LOG)->info("Player received of impulse {} N", impulse);
//  if (r.normalized().dot(gravityVector.normalized()) > std::sqrt(2.0f) / 2.0f) {
//    return SneakStandState{};
//  }
//  return std::nullopt;
//}

} // namespace oo
