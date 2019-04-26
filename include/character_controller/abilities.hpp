#ifndef OPENOBLIVION_CHARACTER_CONTROLLER_ABILITIES_HPP
#define OPENOBLIVION_CHARACTER_CONTROLLER_ABILITIES_HPP

#include "controls.hpp"
#include "character_controller/character_controller_impl.hpp"
#include "character_controller/fallback_state.hpp"
#include <btBulletDynamicsCommon.h>
#include <OgreMath.h>
#include <optional>

namespace oo {

template<class State>
struct MoveAbility {
  std::optional<State>
  handleEvent(CharacterControllerImpl &impl, const event::Forward &event) {
    impl.localVelocity.z -= (event.down ? 1.0f : -1.0f);
    return std::nullopt;
  }

  std::optional<State>
  handleEvent(CharacterControllerImpl &impl, const event::Backward &event) {
    impl.localVelocity.z += (event.down ? 1.0f : -1.0f);
    return std::nullopt;
  }

  std::optional<State>
  handleEvent(CharacterControllerImpl &impl, const event::SlideLeft &event) {
    impl.localVelocity.x -= (event.down ? 1.0f : -1.0f);
    return std::nullopt;
  }

  std::optional<State>
  handleEvent(CharacterControllerImpl &impl, const event::SlideRight &event) {
    impl.localVelocity.x += (event.down ? 1.0f : -1.0f);
    return std::nullopt;
  }
};

template<class State>
struct LookAbility {
  void handleEvent(CharacterControllerImpl &impl, const event::Pitch &event) {
    impl.pitch += -Ogre::Radian(event.delta);
  }

  void handleEvent(CharacterControllerImpl &impl, const event::Yaw &event) {
    impl.yaw += -Ogre::Radian(event.delta);
  }
};

template<class State>
struct CollideAbility {
  std::optional<State> handleCollision(CharacterControllerImpl &/*impl*/,
                                       const btCollisionObject */*other*/,
                                       const btManifoldPoint &/*contact*/) {
    return std::nullopt;
  }
};

} // namespace oo

#endif // OPENOBLIVION_CHARACTER_CONTROLLER_ABILITIES_HPP
