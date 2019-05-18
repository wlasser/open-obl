#include "character_controller/character.hpp"
#include "character_controller/character_mediator.hpp"

namespace oo {

int CharacterMediator::getActorValue(oo::ActorValue actorValue) const noexcept {
  return mCharacter->getActorValue(actorValue);
}

Ogre::Vector3 &CharacterMediator::getLocalVelocity() noexcept {
  return mCharacter->mLocalVelocity;
}

void CharacterMediator::setSpeedModifier(std::function<float(bool, bool)> f) {
  mCharacter->mSpeedModifier = std::move(f);
}

Ogre::Radian &CharacterMediator::getPitch() noexcept {
  return mCharacter->mPitch;
}

Ogre::Radian &CharacterMediator::getYaw() noexcept {
  return mCharacter->mYaw;
}

Ogre::Radian &CharacterMediator::getRootYaw() noexcept {
  return mCharacter->mRootYaw;
}

void CharacterMediator::updateCameraOrientation() noexcept {
  mCharacter->updateCameraOrientation();
}

CharacterMediator::RaycastResult CharacterMediator::raycast() const noexcept {
  return mCharacter->raycast();
}

float CharacterMediator::getHeight() const noexcept {
  return mCharacter->mHeight;
}

bool CharacterMediator::getIsRunning() const noexcept {
  return mCharacter->mIsRunning;
}

void CharacterMediator::setIsRunning(bool isRunning) noexcept {
  mCharacter->mIsRunning = isRunning;
}

} // namespace oo