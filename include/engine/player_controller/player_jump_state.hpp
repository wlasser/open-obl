#ifndef OPENOBLIVION_ENGINE_PLAYER_JUMP_STATE_HPP
#define OPENOBLIVION_ENGINE_PLAYER_JUMP_STATE_HPP

#include "engine/player_controller/abilities.hpp"
#include "engine/player_controller/player_controller.hpp"
#include "engine/player_controller/player_state.hpp"
#include <memory>
#include <variant>

namespace engine {

class PlayerJumpState : public PlayerState,
                        public MoveAbility,
                        public LookAbility {
 public:
  using PlayerState::handleEvent;
  using MoveAbility::handleEvent;
  using LookAbility::handleEvent;

  ~PlayerJumpState() override = default;

  std::shared_ptr<PlayerState>
  handleEvent(PlayerController *player, const KeyVariant &event) override {
    const auto handler = [player, this](auto &&e) {
      return handleEvent(player, e);
    };
    return std::visit(handler, event);
  }

  void
  handleEvent(PlayerController *player, const MouseVariant &event) override {
    const auto handler = [player, this](auto &&e) {
      handleEvent(player, e);
    };
    std::visit(handler, event);
  }

  std::shared_ptr<PlayerState>
  update(PlayerController *player, float elapsed) override;

  std::shared_ptr<PlayerState>
  handleCollision(PlayerController *player,
                  const btCollisionObject *other,
                  const btManifoldPoint &contact) override;

  void enter(PlayerController *player) override;
};

} // namespace engine

#endif // OPENOBLIVION_ENGINE_PLAYER_JUMP_STATE_HPP
