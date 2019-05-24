#ifndef OPENOBL_CHARACTER_CONTROLLER_WALK_STATE_HPP
#define OPENOBL_CHARACTER_CONTROLLER_WALK_STATE_HPP

#include "character_controller/abilities.hpp"
#include "character_controller/character_controller_impl.hpp"
#include "character_controller/fallback_state.hpp"
#include <optional>

namespace oo {

class RunState;

class WalkState : public FallbackState<WalkState>,
                  public CollideAbility<WalkState> {
 public:
  using FallbackState::handleEvent;
  using CollideAbility::handleCollision;

  std::optional<RunState>
  handleEvent(CharacterControllerImpl &impl, const event::Run &event);

  std::optional<RunState>
  handleEvent(CharacterControllerImpl &impl, const event::AlwaysRun &event);

  std::optional<WalkState>
  update(CharacterControllerImpl &impl, float elapsed);

  void enter(CharacterControllerImpl &impl);
  void exit(CharacterControllerImpl &) {}
};

} // namespace oo

#endif // OPENOBL_CHARACTER_CONTROLLER_WALK_STATE_HPP
