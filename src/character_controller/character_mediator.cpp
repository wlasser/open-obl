#include "character_controller/character.hpp"
#include "character_controller/character_mediator.hpp"

namespace oo {

int CharacterMediator::getActorValue(oo::ActorValue actorValue) const noexcept {
  return mCharacter->getActorValue(actorValue);
}

float CharacterMediator::getHeight() const noexcept {
  return mCharacter->mHeight;
}

float CharacterMediator::getMoveSpeed() const noexcept {
  return mCharacter->getMoveSpeed();
}

Ogre::Vector3 &CharacterMediator::getLocalVelocity() noexcept {
  return mCharacter->mLocalVelocity;
}

Ogre::Vector3 &CharacterMediator::getVelocity() noexcept {
  return mCharacter->mVelocity;
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

void CharacterMediator::translate(const Ogre::Vector3 &v) noexcept {
  mCharacter->setPosition(mCharacter->getPosition() + v);
}

bool CharacterMediator::getIsRunning() const noexcept {
  return mCharacter->mIsRunning;
}

void CharacterMediator::setIsRunning(bool isRunning) noexcept {
  mCharacter->mIsRunning = isRunning;
}

void CharacterMediator::updateCamera() noexcept {
  mCharacter->updateCamera();
}

void CharacterMediator::updateCapsule() noexcept {
  mCharacter->updateCapsule();
}

Ogre::Vector4 CharacterMediator::getSurfaceNormal() const noexcept {
  return mCharacter->getSurfaceNormal();
}

Ogre::Matrix3 CharacterMediator::getSurfaceFrame() const noexcept {
  return mCharacter->getSurfaceFrame();
}

std::optional<float> CharacterMediator::getSurfaceDist() const noexcept {
  return mCharacter->getSurfaceDist();
}

Ogre::Matrix3 CharacterMediator::getDefaultFrame() const noexcept {
  return mCharacter->getDefaultFrame();
}
} // namespace oo