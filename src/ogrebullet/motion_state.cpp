#include "ogrebullet/conversions.hpp"
#include "ogrebullet/motion_state.hpp"

namespace Ogre {

MotionState::MotionState(Node *node) : mNode(node) {
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
  worldTrans.setOrigin(conversions::toBullet(mPosition));
  worldTrans.setRotation(conversions::toBullet(mOrientation));
}

void MotionState::setWorldTransform(const btTransform &worldTrans) {
  mPosition = conversions::fromBullet(worldTrans.getOrigin());
  mOrientation = conversions::fromBullet(worldTrans.getRotation());
  mNode->setPosition(mPosition);
  mNode->setOrientation(mOrientation);
}

void MotionState::notify() {
  mPosition = mNode->getPosition();
  mOrientation = mNode->getOrientation();
}

} // namespace Ogre