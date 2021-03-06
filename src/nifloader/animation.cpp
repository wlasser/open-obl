#include "math/conversions.hpp"
#include "nifloader/animation.hpp"
#include "nifloader/nif_resource_manager.hpp"
#include <OgreAnimation.h>
#include <OgreKeyFrame.h>
#include <OgreBone.h>
#include <algorithm>
#include <set>
#include <utility>
#include <vector>

namespace oo {

namespace {

// The root element should be an NiControllerSequence, and will be the first
// element in the block graph.
const nif::NiControllerSequence &getRoot(const BlockGraph &graph) {
  if (boost::num_vertices(graph) == 0) {
    OGRE_EXCEPT(Ogre::Exception::ERR_INVALIDPARAMS,
                "Block graph is empty so has no root",
                "oo::createAnimation");
  }

  const auto &root{*graph[0]};
  if (const auto *ptr{dynamic_cast<const nif::NiControllerSequence *>(&root)}) {
    return *ptr;
  }

  OGRE_EXCEPT(Ogre::Exception::ERR_ITEM_NOT_FOUND,
              "Root node is not an NiControllerSequence",
              "oo::createAnimation()");
}

// Text keys are used to store start and stop time in older formats. Their
// location changes depending on the version.
auto getTextKeys(const nif::NiControllerSequence &controller) {
  if (controller.textKeys) return *controller.textKeys;
  else return *static_cast<const nif::NiSequence &>(controller).textKeys;
}

std::pair<float, float>
getStartStopTime(const BlockGraph &graph,
                 const nif::NiControllerSequence &controller) {
  if (controller.startTime && controller.stopTime) {
    return {*controller.startTime, *controller.stopTime};
  }

  // Controller does not store the times directly, we need to look in the text
  // keys for a 'start' key and a 'stop' key.
  auto textKeysRef{oo::getTextKeys(controller)};

  const auto iVal{static_cast<int32_t>(textKeysRef)};
  if (iVal < 0) return {0.0f, 0.0f};

  const auto val{static_cast<std::size_t>(iVal)};
  if (val >= boost::num_vertices(graph)) return {0.0f, 0.0f};

  const auto &data{dynamic_cast<const nif::NiTextKeyExtraData &>(*graph[val])};
  const auto &keys{data.textKeys};

  auto startIt{std::find_if(keys.begin(), keys.end(), [](const auto &key) {
    return key.value.str() == "start";
  })};
  auto stopIt{std::find_if(keys.begin(), keys.end(), [](const auto &key) {
    return key.value.str() == "stop";
  })};

  return {startIt != keys.end() ? startIt->time : 0.0f,
          stopIt != keys.end() ? stopIt->time : 0.0f};
}

std::string getBoneName(const BlockGraph &graph,
                        const nif::compound::ControlledBlock::Palette &palette) {
  // ver >= 10.2.0.0
  const auto iVal{static_cast<int32_t>(palette.stringPalette)};
  if (iVal < 0) return nullptr;

  const auto val{static_cast<std::size_t>(iVal)};
  if (val >= boost::num_vertices(graph)) return nullptr;

  const auto &block{*graph[val]};
  const auto &p{dynamic_cast<const nif::NiStringPalette &>(block).palette};

  // Offset refers to a character at the start of a null-terminated string.
  auto nodeNameOffset{static_cast<uint32_t>(palette.nodeNameOffset)};
  return std::string(p.palette.value.data() + nodeNameOffset);
}

std::string getBoneName(const BlockGraph &graph,
                        const nif::compound::ControlledBlock &block) {
  if (block.palette) {
    return oo::getBoneName(graph, *block.palette);
    // TODO: Support properties, controllers, etc.
  } else if (block.idTag) {
    // TODO: Support 10.1.0.104 <= ver <= 10.1.0.113
    return "";
  } else {
    // TODO: Support ver < 10.1.0.104
    return block.targetName->str();
  }
}

// TODO: Support older interpolator versions
const nif::NiTransformInterpolator &
getInterpolator(const BlockGraph &graph,
                const nif::compound::ControlledBlock &block) {
  // Interesting(?) aside: this has identical codegen on Clang and GCC to its
  // `goto` equivalent (though then `auto` has to be replaced).
  if (block.interpolator) {
    const auto iVal{static_cast<int32_t>(*block.interpolator)};
    if (iVal > 0) {
      const auto val{static_cast<std::size_t>(iVal)};
      if (val < boost::num_vertices(graph)) {
        return dynamic_cast<const nif::NiTransformInterpolator &>(*graph[val]);
      }
    }
  }

  OGRE_EXCEPT(Ogre::Exception::ERR_ITEM_NOT_FOUND,
              "ControlledBlock does not have an interpolator",
              "oo::createAnimation");
}

Ogre::Vector3 getTranslation(const nif::compound::NiQuatTransform &trans) {
  if (!trans.trsValid || trans.trsValid->at(0)) {
    return oo::fromBSCoordinates(trans.translation);
  }
  return Ogre::Vector3::ZERO;
}

Ogre::Vector3 getScale(const nif::compound::NiQuatTransform &trans) {
  if (!trans.trsValid || trans.trsValid->at(2)) {
    return Ogre::Vector3{trans.scale, trans.scale, trans.scale};
  }
  return Ogre::Vector3::UNIT_SCALE;
}

Ogre::Quaternion getRotation(const nif::compound::NiQuatTransform &trans) {
  if (!trans.trsValid || trans.trsValid->at(1)) {
    return oo::fromBSCoordinates(trans.rotation);
  }
  return Ogre::Quaternion::IDENTITY;
}

const nif::NiTransformData *
getTransformData(const BlockGraph &graph,
                 const nif::NiTransformInterpolator &interpolator) {
  if (!interpolator.data) return nullptr;

  const auto iVal{static_cast<int32_t>(interpolator.data)};
  if (iVal < 0) return nullptr;

  const auto val{static_cast<std::size_t>(iVal)};
  if (val >= boost::num_vertices(graph)) return nullptr;

  return dynamic_cast<const nif::NiTransformData *>(&*graph[val]);
}

} // namespace

Ogre::Animation *createAnimation(Ogre::Skeleton *skeleton,
                                 Ogre::NifResource *nif,
                                 const std::string &animationName) {
  auto blockGraph{nif->getBlockGraph()};
  const auto &controller{oo::getRoot(blockGraph)};

  const auto[startTime, stopTime] = getStartStopTime(blockGraph, controller);
  const float length{stopTime - startTime};

  auto *anim{skeleton->createAnimation(animationName, length)};

  // TODO: Support animation priority and weight

  for (const auto &block : controller.controlledBlocks) {
    std::string boneName{oo::getBoneName(blockGraph, block)};
    const auto &interpolator{oo::getInterpolator(blockGraph, block)};

    // This is the transformation of the bone from the origin to its binding
    // pose. The actual keyframes are *not* given relative to this, rather they
    // already incorporate the binding pose. Since we need the keyframe
    // transformations to be relative, we will apply the inverse of these to
    // each keyframe.
    const auto &trans{interpolator.transform};

    const Ogre::Vector3 translation{oo::getTranslation(trans)};
    const Ogre::Vector3 scale{oo::getScale(trans)};
    const Ogre::Quaternion rotation{oo::getRotation(trans)};

    // getBone throws if the bone doesn't exist, it does not return nullptr.
    if (!skeleton->hasBone(boneName)) continue;
    auto *bone{skeleton->getBone(boneName)};

    if (const auto *data{oo::getTransformData(blockGraph, interpolator)}) {
      // Keyframes are split up by rotation, translation, and scale. They should
      // be ordered by increasing time, but this is not necessary and most
      // (all?) of our data is already ordered so we don't bother with a sort.

      // Scale
      if (data->scales.numKeys > 0) {
        auto *track{anim->createNodeScalingTrack(bone->getHandle(), bone)};
        std::visit([&](const auto &keys) {
          for (const auto &key : keys) {
            auto *kf{track->createScalingKeyFrame(key.time)};
            const Ogre::Vector3 s{key.value, key.value, key.value};
            kf->setScale(s / scale);
          }
        }, data->scales.keys);
      }

      // Translation
      if (data->translations.numKeys > 0) {
        auto *track{anim->createNodeTranslationTrack(bone->getHandle(), bone)};
        std::visit([&](const auto &keys) {
          for (const auto &key : keys) {
            auto *kf{track->createTranslationKeyFrame(key.time)};
            const auto t{oo::fromBSCoordinates(key.value)};
            kf->setTranslate(t - translation);
          }
        }, data->translations.keys);
      }

      // Rotation
      if (data->rotationType != nif::Enum::KeyType::XYZ_ROTATION_KEY) {
        if (data->numRotationKeys > 0) {
          auto *track{anim->createNodeRotationTrack(bone->getHandle(), bone)};
          const auto inv{rotation.Inverse()};
          std::visit([&](const auto &keys) {
            for (const auto &key : keys) {
              auto *kf{track->createRotationKeyFrame(key.time)};
              const auto r{oo::fromBSCoordinates(key.value)};
              kf->setRotation(inv * r);
            }
          }, data->quaternionKeys);
        }
      } else {
        OGRE_EXCEPT(Ogre::Exception::ERR_NOT_IMPLEMENTED,
                    "XYZ rotation keys are not supported",
                    "oo::createAnimation");
      }
    } else {
      // In this case I think the Nif would just transform to the binding pose,
      // but that's the default with Ogre so we don't need to add any frames.
    }
  }

  return anim;
}

Ogre::Animation *createAnimation(Ogre::Skeleton *skeleton,
                                 const std::string &nifName,
                                 const std::string &nifGroup) {
  auto &nifResMgr{Ogre::NifResourceManager::getSingleton()};
  auto nifPtr{nifResMgr.getByName(nifName, nifGroup)};
  if (!nifPtr) {
    OGRE_EXCEPT(Ogre::Exception::ERR_FILE_NOT_FOUND,
                "Cannot locate resource " + nifName + " in resource group "
                    + nifGroup + ".",
                "oo::createAnimation()");
  }
  nifPtr->load();

  return oo::createAnimation(skeleton, nifPtr.get(), nifName);
}

} // namespace oo
