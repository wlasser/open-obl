#include "conversions.hpp"
#include "nifloader/animation.hpp"
#include "nifloader/nif_resource_manager.hpp"
#include <algorithm>
#include <set>
#include <utility>
#include <vector>

namespace oo {

Ogre::Animation *createAnimation(Ogre::Skeleton *skeleton,
                                 Ogre::NifResource *nif) {
  auto blockGraph{nif->getBlockGraph()};

  if (boost::num_vertices(blockGraph) == 0) return nullptr;

  // The root element should be an NiControllerSequence, and will be the first
  // element in the block graph.
  const auto &root{*blockGraph[0]};
  const auto &controller = [&]() {
    if (const auto *ptr{dynamic_cast<const nif::NiControllerSequence *>(&root)};
        ptr) {
      return dynamic_cast<const nif::NiControllerSequence &>(root);
    }
    OGRE_EXCEPT(Ogre::Exception::ERR_ITEM_NOT_FOUND,
                "Root node is not an NiControllerSequence",
                "oo::createAnimation()");
  }();

  // Text keys are used to store start and stop time in older formats. Their
  // location changes depending on the version.
  auto textKeysRef = [&]() {
    if (controller.textKeys) return *controller.textKeys;
    else return *static_cast<const nif::NiSequence &>(controller).textKeys;
  }();

  const std::string animationName{controller.name.str()};

  const auto[startTime, stopTime] = [&]() -> std::pair<float, float> {
    if (controller.startTime && controller.stopTime) {
      return {*controller.startTime, *controller.stopTime};
    }

    const auto iVal{static_cast<int32_t>(textKeysRef)};
    if (iVal < 0) return {0.0f, 0.0f};

    const auto val{static_cast<std::size_t>(iVal)};
    if (val >= boost::num_vertices(blockGraph)) return {0.0f, 0.0f};

    const auto &keys
        {dynamic_cast<const nif::NiTextKeyExtraData &>(*blockGraph[val])
             .textKeys};

    auto startIt{std::find_if(keys.begin(), keys.end(), [](const auto &key) {
      return key.value.str() == "start";
    })};
    auto stopIt{std::find_if(keys.begin(), keys.end(), [](const auto &key) {
      return key.value.str() == "stop";
    })};

    return {startIt != keys.end() ? startIt->time : 0.0f,
            stopIt != keys.end() ? stopIt->time : 0.0f};
  }();

  const float length{stopTime - startTime};

  auto *anim{skeleton->createAnimation(animationName, length)};

  // TODO: Support animation priority and weight

  for (const auto &block : controller.controlledBlocks) {
    std::string nodeName{};

    if (block.palette) {
      // ver >= 10.2.0.0
      const auto iVal{static_cast<int32_t>(block.palette->stringPalette)};
      if (iVal < 0) return nullptr;

      const auto val{static_cast<std::size_t>(iVal)};
      if (val >= boost::num_vertices(blockGraph)) return nullptr;

      const auto &palette
          {dynamic_cast<const nif::NiStringPalette &>(*blockGraph[val])
               .palette};

      auto nodeNameOffset{static_cast<uint32_t>(block.palette->nodeNameOffset)};
      nodeName = std::string(palette.palette.value.data() + nodeNameOffset);

      // TODO: Support properties, controllers, etc.
    } else if (block.idTag) {
      // TODO: Support 10.1.0.104 <= ver <= 10.1.0.113
    } else {
      // TODO: Support ver < 10.1.0.104
      nodeName = block.targetName->str();
    }

    // TODO: Support older interpolator versions
    const nif::NiTransformInterpolator *interpolator{};
    if (block.interpolator) {
      const auto iVal{static_cast<int32_t>(*block.interpolator)};
      if (iVal < 0) return nullptr;

      const auto val{static_cast<std::size_t>(iVal)};
      if (val >= boost::num_vertices(blockGraph)) return nullptr;

      interpolator = dynamic_cast<const nif::NiTransformInterpolator *>(
          &*blockGraph[val]);
    }

    if (!interpolator) return nullptr;

    // This is the transformation of the bone from the origin to its binding
    // pose. The actual keyframes are *not* given relative to this, rather they
    // already incorporate the binding pose. Since we need the keyframe
    // transformations to be relative, we will apply the inverse of these to
    // each keyframe.
    const auto &trans{interpolator->transform};

    const Ogre::Vector3 translation = [&]() {
      if (!trans.trsValid || trans.trsValid->at(0)) {
        return oo::fromBSCoordinates(oo::fromNif(trans.translation));
      }
      return Ogre::Vector3::ZERO;
    }();

    const Ogre::Quaternion rotation = [&]() {
      if (!trans.trsValid || trans.trsValid->at(1)) {
        return oo::fromBSCoordinates(oo::fromNif(trans.rotation));
      }
      return Ogre::Quaternion::IDENTITY;
    }();

    const Ogre::Vector3 scale = [&]() {
      if (!trans.trsValid || trans.trsValid->at(2)) {
        return Ogre::Vector3{trans.scale, trans.scale, trans.scale};
      }
      return Ogre::Vector3::UNIT_SCALE;
    }();

    if (!skeleton->hasBone(nodeName)) continue;

    auto *bone{skeleton->getBone(nodeName)};

    const nif::NiTransformData *transformData{};
    if (interpolator->data) {
      const auto iVal{static_cast<int32_t>(interpolator->data)};
      if (iVal < 0) return nullptr;

      const auto val{static_cast<std::size_t>(iVal)};
      if (val >= boost::num_vertices(blockGraph)) return nullptr;

      transformData = dynamic_cast<const nif::NiTransformData *>(
          &*blockGraph[val]);
    }

    if (transformData) {
      // Keyframes are split up by rotation, translation, and scale. They should
      // be ordered by increasing time, but this is not necessary and most
      // (all?) of our data is already ordered so we don't bother with a sort.

      // Scale
      if (transformData->scales.numKeys > 0) {
        auto *track{anim->createNodeScalingTrack(bone->getHandle(), bone)};
        std::visit([&](const auto &keys) {
          for (const auto &key : keys) {
            const float time{key.time};
            auto *kf{track->createScalingKeyFrame(time)};
            const Ogre::Vector3 s{key.value, key.value, key.value};
            kf->setScale(s / scale);
          }
        }, transformData->scales.keys);
      }

      // Translation
      if (transformData->translations.numKeys > 0) {
        auto *track{anim->createNodeTranslationTrack(bone->getHandle(), bone)};
        std::visit([&](const auto &keys) {
          for (const auto &key : keys) {
            const float time{key.time};
            auto *kf{track->createTranslationKeyFrame(time)};
            const auto t{oo::fromBSCoordinates(oo::fromNif(key.value))};
            kf->setTranslate(t - translation);
          }
        }, transformData->translations.keys);
      }

      // Rotation
      using KeyType = nif::Enum::KeyType;

      if (transformData->rotationType == KeyType::XYZ_ROTATION_KEY) {
        OGRE_EXCEPT(Ogre::Exception::ERR_NOT_IMPLEMENTED,
                    "XYZ rotation keys are not supported",
                    "oo::createAnimation()");
      } else {
        if (transformData->numRotationKeys > 0) {
          auto *track{anim->createNodeRotationTrack(bone->getHandle(), bone)};
          std::visit([&](const auto &keys) {
            for (const auto &key : keys) {
              const float time{key.time};
              auto *kf{track->createRotationKeyFrame(time)};
              const auto r{oo::fromBSCoordinates(oo::fromNif(key.value))};
              kf->setRotation(rotation.Inverse() * r);
            }
          }, transformData->quaternionKeys);
        }
      }
      // End transformData
    } else {
      auto *track{anim->createNodeTrack(bone->getHandle(), bone)};

      auto *k0{track->createNodeKeyFrame(startTime)};
      k0->setTranslate(Ogre::Vector3::ZERO);
      k0->setScale(Ogre::Vector3::UNIT_SCALE);
      k0->setRotation(Ogre::Quaternion::IDENTITY);

      auto *k1{track->createNodeKeyFrame(stopTime)};
      k1->setTranslate(Ogre::Vector3::ZERO);
      k1->setScale(Ogre::Vector3::UNIT_SCALE);
      k1->setRotation(Ogre::Quaternion::IDENTITY);
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

  return oo::createAnimation(skeleton, nifPtr.get());
}

} // namespace oo
