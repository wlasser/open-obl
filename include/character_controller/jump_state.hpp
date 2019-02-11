#ifndef OPENOBLIVION_CHARACTER_CONTROLLER_JUMP_STATE_HPP
#define OPENOBLIVION_CHARACTER_CONTROLLER_JUMP_STATE_HPP

#include "character_controller/abilities.hpp"
#include "character_controller/fallback_state.hpp"
#include "character_controller/player_controller_impl.hpp"
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

  std::optional<StandState> update(PlayerControllerImpl &impl, float elapsed);

  std::optional<StandState>
  handleCollision(PlayerControllerImpl &impl,
                  const btCollisionObject *other,
                  const btManifoldPoint &contact);

  void enter(PlayerControllerImpl &impl);
  void exit(PlayerControllerImpl &) {}
};

} // namespace oo

#endif // OPENOBLIVION_CHARACTER_CONTROLLER_JUMP_STATE_HPP
