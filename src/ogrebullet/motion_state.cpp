#include "ogrebullet/conversions.hpp"
#include "ogrebullet/motion_state.hpp"

namespace Ogre {

MotionState::MotionState(Node *node) : mNode(node) {
  mPosition = mNode->_getDerivedPosition();
  mOrientation = mNode->_getDerivedOrientation();
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
  worldTrans.setOrigin(Ogre::toBullet(mPosition));
  worldTrans.setRotation(Ogre::toBullet(mOrientation));
}

void MotionState::setWorldTransform(const btTransform &worldTrans) {
  mPosition = Ogre::fromBullet(worldTrans.getOrigin());
  mOrientation = Ogre::fromBullet(worldTrans.getRotation());
  mNode->_setDerivedPosition(mPosition);
  mNode->_setDerivedOrientation(mOrientation);
}

void MotionState::notify() {
  mPosition = mNode->_getDerivedPosition();
  mOrientation = mNode->_getDerivedOrientation();
}

} // namespace Ogre
