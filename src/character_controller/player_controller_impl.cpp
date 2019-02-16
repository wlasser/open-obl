#include "character_controller/player_controller_impl.hpp"
#include "settings.hpp"
#include <spdlog/spdlog.h>

namespace oo {

float PlayerControllerImpl::runModifier(float athleticsSkill) const noexcept {
  return *fMoveRunMult + *fMoveRunAthleticsMult * athleticsSkill * 0.01f;
}

float PlayerControllerImpl::swimWalkModifier(float athleticsSkill) const noexcept {
  return *fMoveSwimWalkBase
      + *fMoveSwimWalkAthleticsMult * athleticsSkill * 0.01f;
}

float PlayerControllerImpl::swimRunModifier(float athleticsSkill) const noexcept {
  return *fMoveSwimRunBase
      + *fMoveSwimRunAthleticsMult * athleticsSkill * 0.01f;
}

float PlayerControllerImpl::sneakModifier() const noexcept {
  return *fMoveSneakMult;
}

float PlayerControllerImpl::encumbranceEffectModifier(bool hasWeaponOut) const noexcept {
  return hasWeaponOut ? *fMoveEncumEffect : *fMoveEncumEffectNoWea;
}

float PlayerControllerImpl::encumbranceModifier(float wornWeight,
                                                bool hasWeaponOut) const noexcept {
  const float clampedWornWeight{std::min(wornWeight, *fMoveWeightMax)};
  const float weightRange{std::max(*fMoveWeightMax - *fMoveWeightMin, 0.1f)};
  const float effectMod{encumbranceEffectModifier(hasWeaponOut)};
  const float denominator{*fMoveWeightMin + clampedWornWeight};
  return 1.0f - effectMod * denominator / weightRange;
}

float PlayerControllerImpl::weaponOutModifier(bool hasWeaponOut) const noexcept {
  return hasWeaponOut ? 1.0f : *fMoveNoWeaponMult;
}

float PlayerControllerImpl::baseSpeed(float speedAttribute) const noexcept {
  const float walkRange{*fMoveCharWalkMax - *fMoveCharWalkMin};
  return *fMoveCharWalkMin + walkRange * speedAttribute * 0.01f;
}

float PlayerControllerImpl::runSpeed(float speedAttribute,
                                     float athleticsSkill,
                                     float wornWeight,
                                     float height,
                                     bool hasWeaponOut) const noexcept {
  return baseSpeed(speedAttribute) * runModifier(athleticsSkill)
      * encumbranceModifier(wornWeight, hasWeaponOut) * height
      * oo::metersPerUnit<float>;
}

float PlayerControllerImpl::walkSpeed(float speedAttribute,
                                      float /*athleticsSkill*/,
                                      float wornWeight,
                                      float height,
                                      bool hasWeaponOut) const noexcept {
  return baseSpeed(speedAttribute)
      * encumbranceModifier(wornWeight, hasWeaponOut)
      * height * oo::metersPerUnit<float>;
}

float PlayerControllerImpl::swimRunSpeed(float speedAttribute,
                                         float athleticsSkill,
                                         float wornWeight,
                                         float height,
                                         bool hasWeaponOut) const noexcept {
  return baseSpeed(speedAttribute) * swimRunModifier(athleticsSkill)
      * encumbranceModifier(wornWeight, hasWeaponOut) * height
      * oo::metersPerUnit<float>;
}

float PlayerControllerImpl::swimWalkSpeed(float speedAttribute,
                                          float athleticsSkill,
                                          float wornWeight,
                                          float height,
                                          bool hasWeaponOut) const noexcept {
  return baseSpeed(speedAttribute) * swimWalkModifier(athleticsSkill)
      * encumbranceModifier(wornWeight, hasWeaponOut) * height
      * oo::metersPerUnit<float>;
}

float PlayerControllerImpl::jumpHeight(float acrobaticsSkill) const noexcept {
  const float heightRange{*fJumpHeightMax - *fJumpHeightMin};
  return (*fJumpHeightMin + heightRange * acrobaticsSkill * 0.01f)
      * oo::metersPerUnit<float>;
}

float PlayerControllerImpl::getMoveSpeed() const noexcept {
  const float base{baseSpeed(speedAttribute) * raceHeight
                       * oo::metersPerUnit<float>};
  const float weightMult{encumbranceModifier(wornWeight, hasWeaponOut)};
  return base * weightMult
      * (speedModifier ? speedModifier(hasWeaponOut, isRunning) : 1.0f);
}

float PlayerControllerImpl::getCapsuleRadius() const noexcept {
  return 0.3f;
}

float PlayerControllerImpl::getCapsuleHeight() const noexcept {
  return height * 0.5f - getCapsuleRadius();
}

void PlayerControllerImpl::reactivatePhysics() noexcept {
  rigidBody->activate(true);
}

void PlayerControllerImpl::updateCameraOrientation() noexcept {
  cameraNode->setOrientation(Ogre::Quaternion(Ogre::Radian(0),
                                              Ogre::Vector3::UNIT_X));
  pitchNode->setOrientation(Ogre::Quaternion(Ogre::Radian(0),
                                             Ogre::Vector3::UNIT_X));
  pitchNode->pitch(pitch, Ogre::SceneNode::TS_LOCAL);
  cameraNode->yaw(yaw, Ogre::SceneNode::TS_LOCAL);
}

void PlayerControllerImpl::move() noexcept {
  const auto speed{getMoveSpeed()};
  // This is a rotation of the standard basis, so is still in SO(3)
  const auto axes{cameraNode->getLocalAxes()};
  if (auto len = localVelocity.length(); len > 0.01f) {
    const auto v{rigidBody->getLinearVelocity()};
    auto newV{qvm::convert_to<btVector3>(axes * localVelocity / len * speed)};
    newV.setY(v.y());
    rigidBody->setLinearVelocity(newV);
  } else {
    const auto v{rigidBody->getLinearVelocity()};
    rigidBody->setLinearVelocity({0.0f, v.y(), 0.0f});
  }
}

float PlayerControllerImpl::getSpringDisplacement() noexcept {
  if (!world) return 0.0f;
  using namespace qvm;

  const auto rayLength{10.0f};
  auto p0{qvm::convert_to<btVector3>(motionState->getPosition())};
  auto p1{qvm::convert_to<btVector3>(p0 + qvm::_0X0(-rayLength))};
  btCollisionWorld::AllHitsRayResultCallback callback(p0, p1);
  world->rayTest(p0, p1, callback);

  const int numHits{callback.m_collisionObjects.size()};
  auto dist{rayLength};

  for (int i = 0; i < numHits; ++i) {
    const btCollisionObject *object{callback.m_collisionObjects[i]};
    if (object == rigidBody.get()) continue;

    // No abs needed since p0 is above the collision point by construction.
    const auto distNew{p0.y() - callback.m_hitPointWorld[i].y()};
    if (distNew < dist) dist = distNew;
  }

  // Natural length of the spring
  const auto length{0.5f * height + getCapsuleHeight() / 2.0f};

  return length - dist;
}

float PlayerControllerImpl::getMaxSpringDisplacement() noexcept {
  return 0.5f * height - getCapsuleRadius();
}

void PlayerControllerImpl::applySpringForce(float displacement) noexcept {
  const float deltaMax{getMaxSpringDisplacement()};
  if (displacement < -deltaMax) return;

  const auto k0{4000.0f};
  const auto k1{700.0f};
  const auto v{rigidBody->getLinearVelocity().y()};

  const auto &force{qvm::_0X0(k0 * displacement - k1 * v)};
  rigidBody->applyCentralForce(qvm::convert_to<btVector3>(force));
}

void PlayerControllerImpl::updatePhysics(float /*elapsed*/) noexcept {
  reactivatePhysics();
  updateCameraOrientation();
  move();
}

} // namespace oo
