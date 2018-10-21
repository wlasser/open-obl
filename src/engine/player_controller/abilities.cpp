#include "engine/player_controller/abilities.hpp"
#include "engine/player_controller/player_controller.hpp"
#include <OgreMath.h>

namespace engine {

std::shared_ptr<PlayerState>
MoveAbility::handleEvent(PlayerController *player,
                         const event::Forward &event) {
  player->localVelocity.z -= (event.down ? 1.0f : -1.0f);
  return nullptr;
}

std::shared_ptr<PlayerState>
MoveAbility::handleEvent(PlayerController *player,
                         const event::Backward &event) {
  player->localVelocity.z += (event.down ? 1.0f : -1.0f);
  return nullptr;
}

std::shared_ptr<PlayerState>
MoveAbility::handleEvent(PlayerController *player,
                         const event::SlideLeft &event) {
  player->localVelocity.x -= (event.down ? 1.0f : -1.0f);
  return nullptr;
}

std::shared_ptr<PlayerState>
MoveAbility::handleEvent(PlayerController *player,
                         const event::SlideRight &event) {
  player->localVelocity.x += (event.down ? 1.0f : -1.0f);
  return nullptr;
}

void LookAbility::handleEvent(PlayerController *player,
                              const event::Pitch &event) {
  player->pitch += -Ogre::Radian(event.delta);
}

void LookAbility::handleEvent(PlayerController *player,
                              const event::Yaw &event) {
  player->yaw += -Ogre::Radian(event.delta);
}

} // namespace engine
