#include "conversions.hpp"
#include "nifloader/loader_state.hpp"
#include <stdexcept>

namespace nifloader {

Ogre::Matrix4 getTransform(const nif::NiAVObject &block) {
  using namespace conversions;
  Ogre::Vector3 translation{fromBSCoordinates(fromNif(block.translation))};

  Ogre::Matrix3 rotationMatrix{fromBSCoordinates(fromNif(block.rotation))};
  Ogre::Quaternion rotation{rotationMatrix.transpose()};

  Ogre::Vector3 scale{block.scale, block.scale, block.scale};

  Ogre::Matrix4 trans{};
  trans.makeTransform(translation, scale, rotation);
  return trans;
}

} // namespace nifloader