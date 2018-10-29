#ifndef OPENOBLIVION_ENGINE_CHARACTER_CONTROLLER_JUMP_STATE_HPP
#define OPENOBLIVION_ENGINE_CHARACTER_CONTROLLER_JUMP_STATE_HPP

#include "engine/character_controller/abilities.hpp"
#include "engine/character_controller/fallback_state.hpp"
#include "engine/character_controller/player_controller_impl.hpp"
#include <memory>
#include <variant>

namespace engine::character {

class StandState;

class JumpState : public FallbackState<JumpState>,
                  public MoveAbility<JumpState>,
                  public LookAbility<JumpState> {
 public:
  using FallbackState::handleEvent;
  using MoveAbility::handleEvent;
  using LookAbility::handleEvent;

  std::optional<JumpState> update(PlayerControllerImpl &impl, float elapsed);

  std::optional<StandState>
  handleCollision(PlayerControllerImpl &impl,
                  const btCollisionObject *other,
                  const btManifoldPoint &contact);

  void enter(PlayerControllerImpl &impl);
  void exit(PlayerControllerImpl &) {}
};

} // namespace engine::character

#endif // OPENOBLIVION_ENGINE_CHARACTER_CONTROLLER_JUMP_STATE_HPP
