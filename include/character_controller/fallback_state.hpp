#ifndef OPENOBL_CHARACTER_CONTROLLER_FALLBACK_STATE_HPP
#define OPENOBL_CHARACTER_CONTROLLER_FALLBACK_STATE_HPP

#include "controls.hpp"
#include "character_controller/character_mediator.hpp"
#include <btBulletDynamicsCommon.h>
#include <memory>
#include <optional>
#include <variant>

namespace oo {

using KeyVariant = std::variant<
    event::Forward, event::Backward, event::SlideLeft, event::SlideRight,
    event::Sneak, event::Run, event::AlwaysRun, event::Jump>;
using MouseVariant = std::variant<event::Pitch, event::Yaw>;

template<class State>
class FallbackState {
 public:
  // Fallback for unhandled KeyEvents. Derived classes can write
  // `using FallbackState::handleEvent` to get this automatically.
  std::optional<State>
  handleEvent(CharacterMediator &, const event::KeyEvent &) {
    return std::nullopt;
  }

  // Fallback for unhandled MouseEvents. Derived classes can write
  // `using FallbackState::handleEvent` to get this automatically.
  void handleEvent(CharacterMediator &, const event::MouseEvent &) {}
};

} // namespace oo

#endif // OPENOBL_CHARACTER_CONTROLLER_FALLBACK_STATE_HPP
