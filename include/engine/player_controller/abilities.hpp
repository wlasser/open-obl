#ifndef OPENOBLIVION_ENGINE_ABILITIES_HPP
#define OPENOBLIVION_ENGINE_ABILITIES_HPP

#include "engine/controls.hpp"
#include "engine/player_controller/player_state.hpp"
#include <OgreMath.h>
#include <memory>

namespace engine {

class PlayerController;

struct MoveAbility {
  std::shared_ptr<PlayerState>
  handleEvent(PlayerController *player, const event::Forward &event);

  std::shared_ptr<PlayerState>
  handleEvent(PlayerController *player, const event::Backward &event);

  std::shared_ptr<PlayerState>
  handleEvent(PlayerController *player, const event::SlideLeft &event);

  std::shared_ptr<PlayerState>
  handleEvent(PlayerController *player, const event::SlideRight &event);
};

struct LookAbility {
  void handleEvent(PlayerController *player, const event::Pitch &event);
  void handleEvent(PlayerController *player, const event::Yaw &event);
};

} // namespace engine

#endif // OPENOBLIVION_ENGINE_ABILITIES_HPP
