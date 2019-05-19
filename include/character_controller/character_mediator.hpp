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
  float getHeight() const noexcept;
  float getMoveSpeed() const noexcept;

  Ogre::Vector3 &getLocalVelocity() noexcept;
  Ogre::Vector3 &getVelocity() noexcept;

  void setSpeedModifier(std::function<float(bool, bool)> f);

  Ogre::Radian &getPitch() noexcept;
  Ogre::Radian &getYaw() noexcept;
  Ogre::Radian &getRootYaw() noexcept;

  void translate(const Ogre::Vector3 &v) noexcept;

  bool getIsRunning() const noexcept;
  void setIsRunning(bool isRunning) noexcept;

  void updateCamera() noexcept;
  void updateCapsule() noexcept;

  Ogre::Vector4 getSurfaceNormal() const noexcept;
  Ogre::Matrix3 getSurfaceFrame() const noexcept;
  std::optional<float> getSurfaceDist() const noexcept;
  Ogre::Matrix3 getDefaultFrame() const noexcept;
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
