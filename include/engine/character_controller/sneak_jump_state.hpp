#ifndef OPENOBLIVION_ENGINE_CHARACTER_CONTROLLER_SNEAK_JUMP_STATE_HPP
#define OPENOBLIVION_ENGINE_CHARACTER_CONTROLLER_SNEAK_JUMP_STATE_HPP

#include "engine/character_controller/abilities.hpp"
#include "engine/character_controller/fallback_state.hpp"
#include "engine/character_controller/player_controller_impl.hpp"
#include <optional>
#include <memory>

namespace engine::character {

class SneakStandState;

class SneakJumpState : public FallbackState<SneakJumpState>,
                       public MoveAbility<SneakJumpState>,
                       public LookAbility<SneakJumpState> {
 public:
  using MoveAbility::handleEvent;
  using LookAbility::handleEvent;
  using FallbackState::handleEvent;

  std::optional<SneakJumpState>
  update(PlayerControllerImpl &impl, float elapsed);

  std::optional<SneakStandState>
  handleCollision(PlayerControllerImpl &impl,
                  const btCollisionObject *other,
                  const btManifoldPoint &contact);

  void enter(PlayerControllerImpl &impl);
  void exit(PlayerControllerImpl &impl) {}
};

} // namespace engine::character

#endif // OPENOBLIVION_ENGINE_CHARACTER_CONTROLLER_SNEAK_JUMP_STATE_HPP
