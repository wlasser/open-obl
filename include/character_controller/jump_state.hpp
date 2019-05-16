#ifndef OPENOBL_CHARACTER_CONTROLLER_JUMP_STATE_HPP
#define OPENOBL_CHARACTER_CONTROLLER_JUMP_STATE_HPP

#include "character_controller/abilities.hpp"
#include "character_controller/character_mediator.hpp"
#include "character_controller/fallback_state.hpp"
#include <memory>
#include <variant>

namespace oo {

class StandState;

class JumpState : public FallbackState<JumpState>,
                  public MoveAbility<JumpState>,
                  public LookAbility<JumpState>,
                  public CollideAbility<JumpState> {
 public:
  using FallbackState::handleEvent;
  using MoveAbility::handleEvent;
  using LookAbility::handleEvent;
  using CollideAbility::handleCollision;

  std::optional<StandState> update(CharacterMediator &mediator, float elapsed);

  void enter(CharacterMediator &mediator);
  void exit(CharacterMediator &) {}
};

} // namespace oo

#endif // OPENOBL_CHARACTER_CONTROLLER_JUMP_STATE_HPP
