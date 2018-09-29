#include "engine/conversions.hpp"
#include "engine/nifloader/loader_state.hpp"
#include <stdexcept>

namespace engine::nifloader {

Tagger::Tagger(LoadStatus &tag) : tag(tag) {
  switch (tag) {
    case LoadStatus::Unloaded:tag = LoadStatus::Loading;
      break;
    case LoadStatus::Loading:
      throw std::runtime_error("Cycle detected while loading nif file");
    default: break;
  }
}

Tagger::~Tagger() {
  tag = LoadStatus::Loaded;
}

Ogre::Matrix4 getTransform(nif::NiAVObject *block) {
  using namespace conversions;
  Ogre::Vector3 translation{fromBSCoordinates(fromNif(block->translation))};

  Ogre::Matrix3 rotationMatrix{fromBSCoordinates(fromNif(block->rotation))};
  Ogre::Quaternion rotation{rotationMatrix.transpose()};

  Ogre::Vector3 scale{block->scale, block->scale, block->scale};

  Ogre::Matrix4 trans{};
  trans.makeTransform(translation, scale, rotation);
  return trans;
}

} // namespace engine::nifloader