#ifndef OPENOBLIVION_CHARACTER_CONTROLLER_CHARACTER_MEDIATOR_HPP
#define OPENOBLIVION_CHARACTER_CONTROLLER_CHARACTER_MEDIATOR_HPP

#include "bullet/collision.hpp"
#include "character_controller/movement.hpp"
#include "record/actor_value.hpp"
#include <OgreVector.h>
#include <OgreMath.h>

namespace oo {

class Character;

class CharacterMediator {
 private:
  friend Character;
  Character *mCharacter;

  explicit CharacterMediator(Character *character) noexcept
      : mCharacter(character) {}

 public:
  // TODO: Make states friends instead of making these public? It's not
  //       constructible by non-friends so probably not an issue.

  int getActorValue(oo::ActorValue actorValue) const noexcept;

  Ogre::Vector3 &getLocalVelocity() noexcept;

  void setSpeedModifier(std::function<float(bool, bool)> f);

  Ogre::Radian &getPitch() noexcept;
  Ogre::Radian &getYaw() noexcept;
  Ogre::Radian &getRootYaw() noexcept;

  void updateCameraOrientation() noexcept;
  // TODO: Move this somewhere central to save duplicating the definition.
  using RaycastResult = bullet::ClosestNotMeRayResultCallback;
  RaycastResult raycast() const noexcept;

  float getHeight() const noexcept;

  bool getIsRunning() const noexcept;
  void setIsRunning(bool isRunning) noexcept;
};

inline auto makeSpeedModifier(const CharacterMediator &mediator) {
  return [&mediator](bool hasWeaponOut, bool isRunning) {
    const auto athleticsSkill{mediator.getActorValue(ActorValue::Athletics)};
    const auto weaponMod{oo::weaponOutModifier(hasWeaponOut)};
    return weaponMod * (isRunning ? oo::runModifier(athleticsSkill) : 1.0f);
  };
}

inline auto makeSneakSpeedModifier(const CharacterMediator &mediator) {
  return [&mediator](bool hasWeaponOut, bool isRunning) {
    const auto athleticsSkill{mediator.getActorValue(ActorValue::Athletics)};
    const auto weaponMod{oo::weaponOutModifier(hasWeaponOut)};
    const auto athleticsMod{isRunning ? oo::runModifier(athleticsSkill) : 1.0f};
    return weaponMod * athleticsMod * oo::sneakModifier();
  };
}

} // namespace oo

#endif // OPENOBLIVION_CHARACTER_CONTROLLER_CHARACTER_MEDIATOR_HPP
