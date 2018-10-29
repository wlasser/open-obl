#ifndef OPENOBLIVION_ENGINE_CHARACTER_CONTROLLER_SNEAK_STAND_STATE_HPP
#define OPENOBLIVION_ENGINE_CHARACTER_CONTROLLER_SNEAK_STAND_STATE_HPP

#include "engine/character_controller/abilities.hpp"
#include "engine/character_controller/fallback_state.hpp"
#include "engine/character_controller/player_controller_impl.hpp"
#include <optional>
#include <memory>

namespace engine::character {

class SneakJumpState;
class StandState;

class SneakStandState : public FallbackState<SneakStandState>,
                        public MoveAbility<SneakStandState>,
                        public LookAbility<SneakStandState>,
                        public CollideAbility<SneakStandState> {
 public:
  using MoveAbility::handleEvent;
  using LookAbility::handleEvent;
  using CollideAbility::handleCollision;
  using FallbackState::handleEvent;

  std::optional<SneakJumpState>
  handleEvent(PlayerControllerImpl &impl, const event::Jump &event);

  std::optional<StandState>
  handleEvent(PlayerControllerImpl &impl, const event::Sneak &event);

  std::optional<SneakStandState> update(PlayerControllerImpl &impl,
                                        float elapsed);

  void enter(PlayerControllerImpl &impl);
  void exit(PlayerControllerImpl &impl);
};

} // namespace engine::character

#endif // OPENOBLIVION_ENGINE_CHARACTER_CONTROLLER_SNEAK_STAND_STATE_HPP
