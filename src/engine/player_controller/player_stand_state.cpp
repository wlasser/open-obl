#include "engine/player_controller/player_controller.hpp"
#include "engine/player_controller/player_jump_state.hpp"
#include "engine/player_controller/player_stand_state.hpp"

namespace engine {

std::shared_ptr<PlayerState>
PlayerStandState::handleEvent(PlayerController *player,
                              const event::Jump &event) {
  return std::make_shared<PlayerJumpState>();
}

std::shared_ptr<PlayerState>
PlayerStandState::update(PlayerController *player, float elapsed) {
  player->updatePhysics(elapsed);
  return nullptr;
}

} // namespace engine
