#ifndef OPENOBL_CHARACTER_CONTROLLER_SNEAK_STAND_STATE_HPP
#define OPENOBL_CHARACTER_CONTROLLER_SNEAK_STAND_STATE_HPP

#include "character_controller/abilities.hpp"
#include "character_controller/character_controller_impl.hpp"
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
  handleEvent(CharacterControllerImpl &impl, const event::Jump &event);

  std::optional<StandState>
  handleEvent(CharacterControllerImpl &impl, const event::Sneak &event);

  std::optional<SneakStandState> update(CharacterControllerImpl &impl,
                                        float elapsed);

  void enter(CharacterControllerImpl &impl);
  void exit(CharacterControllerImpl &impl);
};

} // namespace oo

#endif // OPENOBL_CHARACTER_CONTROLLER_SNEAK_STAND_STATE_HPP
