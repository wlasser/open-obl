#include "conversions.hpp"
#include "nifloader/loader_state.hpp"

namespace oo {

Ogre::Matrix4 getTransform(const nif::NiAVObject &block) {
  const Ogre::Vector3 translation{oo::fromBSCoordinates(block.translation)};

  const Ogre::Quaternion rotation = [&block]() {
    const auto m{oo::fromBSCoordinates(block.rotation)};
    return Ogre::Quaternion{m.transpose()};
  }();

  const Ogre::Vector3 scale{block.scale, block.scale, block.scale};

  Ogre::Matrix4 trans;
  trans.makeTransform(translation, scale, rotation);

  return trans;
}

} // namespace oo