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
    qvm::Z(impl.getLocalVelocity()) -= (event.down ? 1.0f : -1.0f);
    return std::nullopt;
  }

  std::optional<State>
  handleEvent(CharacterControllerImpl &impl, const event::Backward &event) {
    qvm::Z(impl.getLocalVelocity()) += (event.down ? 1.0f : -1.0f);
    return std::nullopt;
  }

  std::optional<State>
  handleEvent(CharacterControllerImpl &impl, const event::SlideLeft &event) {
    qvm::X(impl.getLocalVelocity()) -= (event.down ? 1.0f : -1.0f);
    return std::nullopt;
  }

  std::optional<State>
  handleEvent(CharacterControllerImpl &impl, const event::SlideRight &event) {
    qvm::X(impl.getLocalVelocity()) += (event.down ? 1.0f : -1.0f);
    return std::nullopt;
  }
};

template<class State>
struct LookAbility {
  void handleEvent(CharacterControllerImpl &impl, const event::Pitch &event) {
    impl.getPitch() += -Ogre::Radian(event.delta);
  }

  void handleEvent(CharacterControllerImpl &impl, const event::Yaw &event) {
    const Ogre::Radian delta{-event.delta};
    // Clamp camera yaw between [-b, b] and use the excess to move the body.
    const Ogre::Radian bound{Ogre::Math::HALF_PI * 0.5f};
    const Ogre::Radian clampedYaw
        {Ogre::Math::Clamp(delta + impl.getYaw(), -bound, bound)};
    impl.getRootYaw() += delta + impl.getYaw() - clampedYaw;
    impl.getYaw() = clampedYaw;
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
