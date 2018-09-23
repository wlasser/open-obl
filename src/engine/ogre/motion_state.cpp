#include "engine/conversions.hpp"
#include "engine/ogre/motion_state.hpp"

namespace Ogre {

MotionState::MotionState(SceneNode *node) : mNode(node) {
  mPosition = mNode->getPosition();
  mOrientation = mNode->getOrientation();
}

MotionState::MotionState(MotionState &&other) noexcept {
  using std::swap;
  swap(other.mNode, mNode);
  swap(other.mPosition, mPosition);
  swap(other.mOrientation, mOrientation);
  // other.mNode should be nullptr as this->mNode has been default initialized
  // to nullptr already, but just to be sure
  other.mNode = nullptr;
}

MotionState &MotionState::operator=(MotionState &&other) noexcept {
  MotionState tmp{std::move(other)};
  std::swap(other, *this);
  return *this;
}

void MotionState::getWorldTransform(btTransform &worldTrans) const {
  worldTrans.setIdentity();
  worldTrans.setOrigin(engine::conversions::toBullet(mPosition));
  worldTrans.setRotation(engine::conversions::toBullet(mOrientation));
}

void MotionState::setWorldTransform(const btTransform &worldTrans) {
  mPosition = engine::conversions::fromBullet(worldTrans.getOrigin());
  mOrientation = engine::conversions::fromBullet(worldTrans.getRotation());
  mNode->setPosition(mPosition);
  mNode->setOrientation(mOrientation);
}

void MotionState::notify() {
  mPosition = mNode->getPosition();
  mOrientation = mNode->getOrientation();
}

} // namespace Ogre