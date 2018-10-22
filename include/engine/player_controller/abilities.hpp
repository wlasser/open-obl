#ifndef OPENOBLIVION_ENGINE_ABILITIES_HPP
#define OPENOBLIVION_ENGINE_ABILITIES_HPP

#include "engine/controls.hpp"
#include "engine/player_controller/player_state.hpp"
#include "engine/player_controller/player_controller_impl.hpp"
#include <OgreMath.h>
#include <optional>

namespace engine {

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

} // namespace engine

#endif // OPENOBLIVION_ENGINE_ABILITIES_HPP
