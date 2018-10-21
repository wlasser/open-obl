#include "engine/player_controller/player_controller.hpp"
#include "engine/player_controller/player_jump_state.hpp"
#include "engine/settings.hpp"
#include <btBulletDynamicsCommon.h>
#include <spdlog/spdlog.h>
#include <cmath>
#include <memory>

namespace engine {

std::shared_ptr<PlayerState>
PlayerJumpState::update(PlayerController *player, float elapsed) {
  player->updatePhysics(elapsed);
  return nullptr;
}

void PlayerJumpState::enter(PlayerController *player) {
  // Player jumps in the opposite direction of gravity, with an impulse chosen
  // to give the desired jump height. To find the impulse, use v^2 = u^2 + 2as
  // along with the fact that the impulse is the change in momentum.
  const btVector3 gravityVector{player->rigidBody->getGravity()};
  const float g{gravityVector.length()};
  const float apex{player->jumpHeight(player->acrobaticsSkill)};
  const float impulse{player->mass * std::sqrt(2.0f * g * apex)};
  player->rigidBody->applyCentralImpulse(-impulse * gravityVector.normalized());
}

std::shared_ptr<PlayerState>
PlayerJumpState::handleCollision(PlayerController *player,
                                 const btCollisionObject *other,
                                 const btManifoldPoint &contact) {
  const auto impulse{contact.getAppliedImpulse()};
  const auto r{contact.getPositionWorldOnA() - contact.getPositionWorldOnB()};
  spdlog::get(settings::log)->info("Player received of impulse {} N", impulse);
  if (r.normalized().dot(player->rigidBody->getGravity().normalized()) > 0.7) {
    return std::make_shared<PlayerStandState>();
  }
  return nullptr;
}

} // namespace engine