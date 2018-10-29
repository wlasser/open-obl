#ifndef OPENOBLIVION_ENGINE_CHARACTER_CONTROLLER_ABILITIES_HPP
#define OPENOBLIVION_ENGINE_CHARACTER_CONTROLLER_ABILITIES_HPP

#include "engine/controls.hpp"
#include "engine/character_controller/fallback_state.hpp"
#include "engine/character_controller/player_controller_impl.hpp"
#include <btBulletDynamicsCommon.h>
#include <OgreMath.h>
#include <optional>

namespace engine::character {

template<class State>
struct MoveAbility {
  std::optional<State>
  handleEvent(PlayerControllerImpl &impl, const event::Forward &event) {
    impl.localVelocity.z -= (event.down ? 1.0f : -1.0f);
    return std::nullopt;
  }

  std::optional<State>
  handleEvent(PlayerControllerImpl &impl, const event::Backward &event) {
    impl.localVelocity.z += (event.down ? 1.0f : -1.0f);
    return std::nullopt;
  }

  std::optional<State>
  handleEvent(PlayerControllerImpl &impl, const event::SlideLeft &event) {
    impl.localVelocity.x -= (event.down ? 1.0f : -1.0f);
    return std::nullopt;
  }

  std::optional<State>
  handleEvent(PlayerControllerImpl &impl, const event::SlideRight &event) {
    impl.localVelocity.x += (event.down ? 1.0f : -1.0f);
    return std::nullopt;
  }
};

template<class State>
struct LookAbility {
  void handleEvent(PlayerControllerImpl &impl, const event::Pitch &event) {
    impl.pitch += -Ogre::Radian(event.delta);
  }

  void handleEvent(PlayerControllerImpl &impl, const event::Yaw &event) {
    impl.yaw += -Ogre::Radian(event.delta);
  }
};

template<class State>
struct CollideAbility {
  std::optional<State> handleCollision(PlayerControllerImpl &/*impl*/,
                                       const btCollisionObject */*other*/,
                                       const btManifoldPoint &/*contact*/) {
    return std::nullopt;
  }
};

} // namespace engine::character

#endif // OPENOBLIVION_ENGINE_CHARACTER_CONTROLLER_ABILITIES_HPP
