#ifndef OPENOBL_CHARACTER_CONTROLLER_RUN_STATE_HPP
#define OPENOBL_CHARACTER_CONTROLLER_RUN_STATE_HPP

#include "character_controller/abilities.hpp"
#include "character_controller/character_controller_impl.hpp"
#include "character_controller/fallback_state.hpp"
#include <optional>

namespace oo {

class WalkState;

class RunState : public FallbackState<RunState>,
                 public CollideAbility<RunState> {
 public:
  using FallbackState::handleEvent;
  using CollideAbility::handleCollision;

  std::optional<WalkState>
  handleEvent(CharacterControllerImpl &impl, const event::Run &event);

  std::optional<WalkState>
  handleEvent(CharacterControllerImpl &impl, const event::AlwaysRun &event);

  std::optional<RunState>
  update(CharacterControllerImpl &impl, float elapsed);

  void enter(CharacterControllerImpl &impl);
  void exit(CharacterControllerImpl &) {}
};

} // namespace oo

#endif // OPENOBL_CHARACTER_CONTROLLER_RUN_STATE_HPP
