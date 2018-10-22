#ifndef OPENOBLIVION_ENGINE_PLAYER_STATE_HPP
#define OPENOBLIVION_ENGINE_PLAYER_STATE_HPP

#include "engine/controls.hpp"
#include <btBulletDynamicsCommon.h>
#include <memory>
#include <optional>
#include <variant>

namespace engine {

using KeyVariant = std::variant<
    event::Forward, event::Backward, event::SlideLeft, event::SlideRight,
    event::Sneak, event::Jump>;
using MouseVariant = std::variant<event::Pitch, event::Yaw>;

template<class State>
class PlayerState {
 public:
  // Fallback for unhandled KeyEvents. Derived classes can write
  // `using PlayerState::handleEvent` to get this automatically.
  std::optional<State>
  handleEvent(PlayerControllerImpl &, const event::KeyEvent &) {
    return std::nullopt;
  }

  // Fallback for unhandled MouseEvents. Derived classes can write
  // `using PlayerState::handleEvent` to get this automatically.
  void handleEvent(PlayerControllerImpl &, const event::MouseEvent &) {}

  std::optional<State> update(PlayerControllerImpl &player, float elapsed) {
    return std::nullopt;
  }

  std::optional<State> handleCollision(PlayerControllerImpl &player,
                                       const btCollisionObject *other,
                                       const btManifoldPoint &contact) {
    return std::nullopt;
  }

  void enter(PlayerControllerImpl &player) {}
  void exit(PlayerControllerImpl &player) {}
};

} // namespace engine

#endif // OPENOBLIVION_ENGINE_PLAYER_STATE_HPP
