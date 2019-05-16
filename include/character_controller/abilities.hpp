#ifndef OPENOBL_CHARACTER_CONTROLLER_ABILITIES_HPP
#define OPENOBL_CHARACTER_CONTROLLER_ABILITIES_HPP

#include "controls.hpp"
#include "character_controller/character_mediator.hpp"
#include "character_controller/fallback_state.hpp"
#include "math/conversions.hpp"
#include <OgreMath.h>
#include <optional>
#include "util/windows_cleanup.hpp"

namespace oo {

template<class State>
struct MoveAbility {
  std::optional<State>
  handleEvent(CharacterMediator &mediator, const event::Forward &event) {
    qvm::Z(mediator.getLocalVelocity()) -= (event.down ? 1.0f : -1.0f);
    return std::nullopt;
  }

  std::optional<State>
  handleEvent(CharacterMediator &mediator, const event::Backward &event) {
    qvm::Z(mediator.getLocalVelocity()) += (event.down ? 1.0f : -1.0f);
    return std::nullopt;
  }

  std::optional<State>
  handleEvent(CharacterMediator &mediator, const event::SlideLeft &event) {
    qvm::X(mediator.getLocalVelocity()) -= (event.down ? 1.0f : -1.0f);
    return std::nullopt;
  }

  std::optional<State>
  handleEvent(CharacterMediator &mediator, const event::SlideRight &event) {
    qvm::X(mediator.getLocalVelocity()) += (event.down ? 1.0f : -1.0f);
    return std::nullopt;
  }
};

template<class State>
struct LookAbility {
  void handleEvent(CharacterMediator &mediator, const event::Pitch &event) {
    mediator.getPitch() += -Ogre::Radian(event.delta);
  }

  void handleEvent(CharacterMediator &mediator, const event::Yaw &event) {
    const Ogre::Radian delta{-event.delta};
    // Clamp camera yaw between [-b, b] and use the excess to move the body.
    const Ogre::Radian bound{Ogre::Math::HALF_PI * 0.5f};
    const Ogre::Radian clampedYaw
        {Ogre::Math::Clamp(delta + mediator.getYaw(), -bound, bound)};
    mediator.getRootYaw() += delta + mediator.getYaw() - clampedYaw;
    mediator.getYaw() = clampedYaw;
  }
};

template<class State>
struct CollideAbility {
  std::optional<State> handleCollision(CharacterMediator &/*mediator*/,
                                       const btCollisionObject */*other*/,
                                       const btManifoldPoint &/*contact*/) {
    return std::nullopt;
  }
};

} // namespace oo

#endif // OPENOBL_CHARACTER_CONTROLLER_ABILITIES_HPP
