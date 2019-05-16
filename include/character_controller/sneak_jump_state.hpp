#ifndef OPENOBL_CHARACTER_CONTROLLER_SNEAK_JUMP_STATE_HPP
#define OPENOBL_CHARACTER_CONTROLLER_SNEAK_JUMP_STATE_HPP

#include "character_controller/abilities.hpp"
#include "character_controller/character_mediator.hpp"
#include "character_controller/fallback_state.hpp"
#include <optional>
#include <memory>

namespace oo {

class SneakStandState;

class SneakJumpState : public FallbackState<SneakJumpState>,
                       public MoveAbility<SneakJumpState>,
                       public LookAbility<SneakJumpState>,
                       public CollideAbility<SneakJumpState> {
 public:
  using MoveAbility::handleEvent;
  using LookAbility::handleEvent;
  using FallbackState::handleEvent;
  using CollideAbility::handleCollision;

  std::optional<SneakStandState>
  update(CharacterMediator &mediator, float elapsed);

  void enter(CharacterMediator &mediator);
  void exit(CharacterMediator &) {}
};

} // namespace oo

#endif // OPENOBL_CHARACTER_CONTROLLER_SNEAK_JUMP_STATE_HPP
