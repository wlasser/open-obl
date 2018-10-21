#ifndef OPENOBLIVION_ENGINE_PLAYER_STATE_HPP
#define OPENOBLIVION_ENGINE_PLAYER_STATE_HPP

#include "engine/controls.hpp"
#include <btBulletDynamicsCommon.h>
#include <memory>
#include <variant>

namespace engine {

using KeyVariant = std::variant<
    event::Forward, event::Backward, event::SlideLeft, event::SlideRight,
    event::Jump>;
using MouseVariant = std::variant<event::Pitch, event::Yaw>;

class PlayerController;

class PlayerState {
 public:
  virtual ~PlayerState() = 0;

  // Handle the key press/release event, returning a new state to transition to
  // if required, and nullptr otherwise.
  virtual std::shared_ptr<PlayerState>
  handleEvent(PlayerController *player, const KeyVariant &event) {
    return nullptr;
  }

  // Handle the mouse motion event. No state transitions are allowed.
  virtual void
  handleEvent(PlayerController *player, const MouseVariant &event) {}

  // Fallback for unhandled KeyEvents. Derived classes can write
  // `using PlayerState::handleEvent` to get this automatically.
  std::shared_ptr<PlayerState>
  handleEvent(PlayerController *, const event::KeyEvent &) {
    return nullptr;
  }

  // Fallback for unhandled MouseEvents. Derived classes can write
  // `using PlayerState::handleEvent` to get this automatically.
  void handleEvent(PlayerController *, const event::MouseEvent &) {}

  virtual std::shared_ptr<PlayerState>
  update(PlayerController *player, float elapsed) = 0;

  virtual std::shared_ptr<PlayerState>
  handleCollision(PlayerController *player,
                  const btCollisionObject *other,
                  const btManifoldPoint &contact) {
    return nullptr;
  }

  virtual void enter(PlayerController *player) {}
};

inline PlayerState::~PlayerState() = default;

} // namespace engine

#endif // OPENOBLIVION_ENGINE_PLAYER_STATE_HPP
