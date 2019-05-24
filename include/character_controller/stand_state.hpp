#ifndef OPENOBL_CHARACTER_CONTROLLER_STAND_STATE_HPP
#define OPENOBL_CHARACTER_CONTROLLER_STAND_STATE_HPP

#include "character_controller/abilities.hpp"
#include "character_controller/character_controller_impl.hpp"
#include "character_controller/fallback_state.hpp"
#include <optional>
#include <variant>

namespace oo {

class JumpState;
class SneakStandState;

class StandState : public FallbackState<StandState>,
                   public MoveAbility<StandState>,
                   public LookAbility<StandState>,
                   public CollideAbility<StandState> {
 public:
  using MoveAbility::handleEvent;
  using LookAbility::handleEvent;
  using CollideAbility::handleCollision;
  using FallbackState::handleEvent;

  std::optional<JumpState>
  handleEvent(CharacterControllerImpl &impl, const event::Jump &event);

  std::optional<SneakStandState>
  handleEvent(CharacterControllerImpl &impl, const event::Sneak &event);

  std::optional<StandState>
  update(CharacterControllerImpl &impl, float elapsed);

  void enter(CharacterControllerImpl &impl);
  void exit(CharacterControllerImpl &) {}
};

} // namespace oo

#endif // OPENOBL_CHARACTER_CONTROLLER_STAND_STATE_HPP
