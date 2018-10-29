#ifndef OPENOBLIVION_ENGINE_CHARACTER_CONTROLLER_FALLBACK_STATE_HPP
#define OPENOBLIVION_ENGINE_CHARACTER_CONTROLLER_FALLBACK_STATE_HPP

#include "engine/controls.hpp"
#include "engine/character_controller/player_controller_impl.hpp"
#include <btBulletDynamicsCommon.h>
#include <memory>
#include <optional>
#include <variant>

namespace engine::character {

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
  handleEvent(PlayerControllerImpl &, const event::KeyEvent &) {
    return std::nullopt;
  }

  // Fallback for unhandled MouseEvents. Derived classes can write
  // `using FallbackState::handleEvent` to get this automatically.
  void handleEvent(PlayerControllerImpl &, const event::MouseEvent &) {}
};

} // namespace engine::character

#endif // OPENOBLIVION_ENGINE_CHARACTER_CONTROLLER_FALLBACK_STATE_HPP
