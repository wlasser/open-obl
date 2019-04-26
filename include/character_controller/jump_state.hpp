#ifndef OPENOBLIVION_CHARACTER_CONTROLLER_JUMP_STATE_HPP
#define OPENOBLIVION_CHARACTER_CONTROLLER_JUMP_STATE_HPP

#include "character_controller/abilities.hpp"
#include "character_controller/character_controller_impl.hpp"
#include "character_controller/fallback_state.hpp"
#include <memory>
#include <variant>

namespace oo {

class StandState;

class JumpState : public FallbackState<JumpState>,
                  public MoveAbility<JumpState>,
                  public LookAbility<JumpState> {
 public:
  using FallbackState::handleEvent;
  using MoveAbility::handleEvent;
  using LookAbility::handleEvent;

  std::optional<StandState> update(CharacterControllerImpl &impl,
                                   float elapsed);

  std::optional<StandState>
  handleCollision(CharacterControllerImpl &impl,
                  const btCollisionObject *other,
                  const btManifoldPoint &contact);

  void enter(CharacterControllerImpl &impl);
  void exit(CharacterControllerImpl &) {}
};

} // namespace oo

#endif // OPENOBLIVION_CHARACTER_CONTROLLER_JUMP_STATE_HPP
