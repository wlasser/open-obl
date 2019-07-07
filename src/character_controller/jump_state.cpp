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
  mediator.updateCamera();

  const auto localVelocity{mediator.getLocalVelocity()};
  Ogre::Vector3 &velocity{mediator.getVelocity()};

  if (const auto localSpeed{localVelocity.length()}; localSpeed > 0.01f) {
    const auto speed{mediator.getMoveSpeed()};
    const Ogre::Matrix3 frame{mediator.getDefaultFrame()};
    const auto yVel{qvm::Y(velocity)};
    velocity = frame * localVelocity / localSpeed * speed;
    velocity.y = yVel;
  }

  if (qvm::Y(velocity) <= 0.01f) {
    const auto dist{mediator.getSurfaceDist()};
    if (dist && dist <= 0.1f) {
      // TODO: Apply separating vector (0, -dist, 0) to correct for penetration.
      return std::make_optional<StandState>();
    }
  }

  qvm::Y(velocity) -= 9.81f * elapsed;

  mediator.translate(velocity * elapsed);

  mediator.updateCapsule();

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