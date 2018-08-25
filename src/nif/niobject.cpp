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

void AbstractAdditionalGeometryData::read(std::istream &is) {
  NiObject::read(is);
}

void NiAdditionalGeometryData::read(std::istream &is) {
  AbstractAdditionalGeometryData::read(is);
  io::readBytes(is, numVertices);

  io::readBytes(is, numBlockInfos);
  blockInfos.reserve(numBlockInfos);
  for (auto i = 0; i < numBlockInfos; ++i) {
    blockInfos.emplace_back();
    is >> blockInfos.back();
  }

  io::readBytes(is, numBlocks);
  blocks.reserve(static_cast<std::size_t>(numBlocks));
  for (auto i = 0; i < numBlocks; ++i) {
    blocks.emplace_back();
    is >> blocks.back();
  }
}

void NiGeometryData::read(std::istream &is) {
  NiObject::read(is);

  io::readBytes(is, groupID);
  io::readBytes(is, numVertices);
  io::readBytes(is, keepFlags);
  io::readBytes(is, compressFlags);

  io::readBytes(is, hasVertices);
  vertices.reserve(numVertices);
  for (auto i = 0; i < numVertices; ++i) {
    vertices.emplace_back();
    is >> vertices.back();
  }

  io::readBytes(is, vectorFlags);

  io::readBytes(is, hasNormals);
  normals.reserve(numVertices);
  for (auto i = 0; i < numVertices; ++i) {
    normals.emplace_back();
    is >> normals.back();
  }

  if (hasNormals && vectorFlags
      && static_cast<uint16_t>(*vectorFlags
          & Enum::VectorFlags::VF_Has_Tangents) != 0) {
    tangents.reserve(numVertices);
    for (auto i = 0; i < numVertices; ++i) {
      tangents.emplace_back();
      is >> tangents.back();
    }

    bitangents.reserve(numVertices);
    for (auto i = 0; i < numVertices; ++i) {
      bitangents.emplace_back();
      is >> bitangents.back();
    }
  }

  is >> center;
  io::readBytes(is, radius);

  io::readBytes(is, hasVertexColors);
  vertexColors.reserve(numVertices);
  for (auto i = 0; i < numVertices; ++i) {
    vertexColors.emplace_back();
    is >> vertexColors.back();
  }

  io::readBytes(is, numUVSets);
  io::readBytes(is, hasUV);
  if (numUVSets && hasUV) {
    std::size_t arr1 = static_cast<uint16_t>(*numUVSets & 0b11111)
        | static_cast<uint16_t>(*vectorFlags & Enum::VectorFlags::VF_UV_MASK);
    for (auto i = 0; i < arr1; ++i) {
      std::vector<compound::TexCoord> row{};
      for (auto j = 0; j < numVertices; ++j) {
        row.emplace_back();
        is >> row.back();
      }
      uvSets.push_back(row);
    }
  }

  io::readBytes(is, consistencyFlags);
  io::readBytes(is, additionalData);
}
} // namespace nif
