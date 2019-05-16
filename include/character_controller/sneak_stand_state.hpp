#ifndef OPENOBL_CHARACTER_CONTROLLER_SNEAK_STAND_STATE_HPP
#define OPENOBL_CHARACTER_CONTROLLER_SNEAK_STAND_STATE_HPP

#include "character_controller/abilities.hpp"
#include "character_controller/character_mediator.hpp"
#include "character_controller/fallback_state.hpp"
#include <optional>
#include <memory>

namespace oo {

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
  handleEvent(CharacterMediator &mediator, const event::Jump &event);

  std::optional<StandState>
  handleEvent(CharacterMediator &mediator, const event::Sneak &event);

  std::optional<SneakStandState>
  update(CharacterMediator &mediator, float elapsed);

  void enter(CharacterMediator &mediator);
  void exit(CharacterMediator &mediator);
};

} // namespace oo

#endif // OPENOBL_CHARACTER_CONTROLLER_SNEAK_STAND_STATE_HPP
