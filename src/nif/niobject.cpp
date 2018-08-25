#include "io/read_bytes.hpp"
#include "nif/niobject.hpp"

namespace nif {

void NiExtraData::read(std::istream &is) {
  NiObject::read(is);
  io::readBytes(is, name);
  is >> next;
}

void NiTimeController::read(std::istream &is) {
  NiObject::read(is);
  io::readBytes(is, flags);
  io::readBytes(is, frequency);
  io::readBytes(is, phase);
  io::readBytes(is, startTime);
  io::readBytes(is, stopTime);
  io::readBytes(is, controllerTarget);
}

void NiObjectNet::read(std::istream &is) {
  NiObject::read(is);
  is >> name;
  is >> extraData;
  io::readBytes(is, extraDataArrayLength);
  if (extraDataArrayLength && extraDataArray) {
    extraDataArray->reserve(extraDataArrayLength.value());
    for (auto i = 0; i < extraDataArrayLength.value(); ++i) {
      extraDataArray->emplace_back();
      is >> extraDataArray->back();
    }
  }
  io::readBytes(is, controller);
}

void NiProperty::read(std::istream &is) {
  NiObjectNet::read(is);
}

void NiAVObject::read(std::istream &is) {
  NiObjectNet::read(is);
  io::readBytes(is, flags);
  is >> translation >> rotation;
  io::readBytes(is, scale);
  is >> velocity;
  io::readBytes(is, numProperties);
  properties.reserve(numProperties);
  for (auto i = 0; i < numProperties; ++i) {
    properties.emplace_back();
    is >> properties.back();
  }
  io::readBytes(is, hasBoundingVolume);
  is >> boundingVolume >> collisionObject;
}

void NiCollisionObject::read(std::istream &is) {
  NiObject::read(is);
  is >> target;
}

void NiDynamicEffect::read(std::istream &is) {
  NiAVObject::read(is);
  io::readBytes(is, switchState);
  if (!noAffectedNodes) {
    io::readBytes(is, numAffectedNodes);
    affectedNodes.reserve(numAffectedNodes);
    for (auto i = 0; i < numAffectedNodes; ++i) {
      affectedNodes.emplace_back();
      is >> affectedNodes.back();
    }
  }
}

void NiNode::read(std::istream &is) {
  NiAVObject::read(is);
  io::readBytes(is, numChildren);
  children.reserve(numChildren);
  for (auto i = 0; i < numChildren; ++i) {
    children.emplace_back();
    is >> children.back();
  }
  io::readBytes(is, numEffects);
  effects.reserve(numEffects);
  for (auto i = 0; i < numEffects; ++i) {
    effects.emplace_back();
    is >> effects.back();
  }
}

void NiSkinPartition::read(std::istream &is) {
  NiObject::read(is);
  io::readBytes(is, numSkinPartitionBlocks);
  skinPartitionBlocks.reserve(numSkinPartitionBlocks);
  for (auto i = 0; i < numSkinPartitionBlocks; ++i) {
    skinPartitionBlocks.emplace_back(version);
    is >> skinPartitionBlocks.back();
  }
}

void NiSkinData::read(std::istream &is) {
  NiObject::read(is);
  is >> skinTransform;
  io::readBytes(is, numBones);
  is >> skinPartition;
  io::readBytes(is, hasVertexWeights);
  boneList.reserve(numBones);
  for (auto i = 0; i < numBones; ++i) {
    BoneData bone{};
    is >> bone.skinTransform >> bone.boundingSphereOffset;
    io::readBytes(is, bone.boundingSphereRadius);
    io::readBytes(is, bone.numVertices);
    if (hasVertexWeights) {
      bone.vertexWeights.reserve(bone.numVertices);
      for (auto j = 0; j < bone.numVertices; ++j) {
        bone.vertexWeights.emplace_back();
        is >> bone.vertexWeights.back();
      }
    }
    boneList.push_back(bone);
  }
}

void NiSkinInstance::read(std::istream &is) {
  NiObject::read(is);
  is >> skinPartition >> skeletonRoot;
  io::readBytes(is, numBones);
  bones.reserve(numBones);
  for (auto i = 0; i < numBones; ++i) {
    bones.emplace_back();
    is >> bones.back();
  }
}

void NiGeometry::read(std::istream &is) {
  NiAVObject::read(is);
  is >> data >> skinInstance >> materialData;
}

void NiTriBasedGeom::read(std::istream &is) {
  NiGeometry::read(is);
}

void NiTriShape::read(std::istream &is) {
  NiTriBasedGeom::read(is);
}

} // namespace nif
