#include "io/read_bytes.hpp"
#include "nif/niobject.hpp"

namespace nif {

void NiFloatData::read(std::istream &is) {
  NiObject::read(is);
  is >> keys;
}

void NiKeyframeData::read(std::istream &is) {
  NiObject::read(is);
  io::readBytes(is, numRotationKeys);
  if (numRotationKeys != 0) {
    io::readBytes(is, rotationType);
    switch (static_cast<std::size_t>(rotationType)) {
      case 0: readKeys<0>(is);
        break;
      case 1: readKeys<1>(is);
        break;
      case 2: readKeys<2>(is);
        break;
      case 3: readKeys<3>(is);
        break;
      case 4: io::readBytes(is, order);
        for (int i = 0; i < 3; ++i) is >> xyzRotations[i];
        break;
      default:
        throw std::runtime_error(boost::str(
            boost::format("Expected a KeyType, found %d") %
                static_cast<uint32_t>(rotationType)));
    }
  }
  is >> translations;
  is >> scales;
}

void NiTransformData::read(std::istream &is) {
  NiKeyframeData::read(is);
}

void NiPosData::read(std::istream &is) {
  NiObject::read(is);
  is >> data;
}

void NiStringPalette::read(std::istream &is) {
  NiObject::read(is);
  is >> palette;
}

void NiExtraData::read(std::istream &is) {
  NiObject::read(is);
  is >> name;
  is >> next;
}

void NiBinaryExtraData::read(std::istream &is) {
  NiExtraData::read(is);
  is >> data;
}

void NiIntegerExtraData::read(std::istream &is) {
  NiExtraData::read(is);
  io::readBytes(is, data);
}

void NiStringExtraData::read(std::istream &is) {
  NiExtraData::read(is);
  io::readBytes(is, bytesRemaining);
  is >> data;
}

void NiTextKeyExtraData::read(std::istream &is) {
  NiExtraData::read(is);
  io::readBytes(is, unknownInt1);
  io::readBytes(is, numTextKeys);
  textKeys.reserve(numTextKeys);
  for (auto i = 0; i < numTextKeys; ++i) is >> textKeys.emplace_back();
}

void NiInterpolator::read(std::istream &is) {
  NiObject::read(is);
}

void NiKeyBasedInterpolator::read(std::istream &is) {
  NiInterpolator::read(is);
}

void NiFloatInterpolator::read(std::istream &is) {
  NiKeyBasedInterpolator::read(is);
  io::readBytes(is, value);
  is >> data;
}

void NiTransformInterpolator::read(std::istream &is) {
  NiKeyBasedInterpolator::read(is);
  is >> transform;
  is >> data;
}

void NiPoint3Interpolator::read(std::istream &is) {
  NiKeyBasedInterpolator::read(is);
  is >> value;
  is >> data;
}

void NiBlendInterpolator::read(std::istream &is) {
  NiInterpolator::read(is);
  io::readBytes(is, flags);
  is >> arrayParams;
  if (weightThresholdR) io::readBytes(is, *weightThresholdR);
  if (unmanagedData && flags && (static_cast<uint8_t>(*flags) & 1) == 0) {
    io::readBytes(is, unmanagedData->interpCount);
    io::readBytes(is, unmanagedData->singleIndex);
    io::readBytes(is, unmanagedData->highPriority);
    io::readBytes(is, unmanagedData->nextHighPriority);
    io::readBytes(is, unmanagedData->singleTime);
    io::readBytes(is, unmanagedData->highWeightsSum);
    io::readBytes(is, unmanagedData->nextHighWeightsSum);
    io::readBytes(is, unmanagedData->highEaseSpinner);
    // TODO: Visit
    if (arrayParams) {
      auto size = static_cast<std::size_t>(arrayParams.value<true>().arraySize);
      unmanagedData->interpArrayItems.reserve(size);
      for (auto i = 0; i < size; ++i) {
        is >> unmanagedData->interpArrayItems.emplace_back(version);
      }
    } else {
      auto size = arrayParams.value<false>().arraySize;
      unmanagedData->interpArrayItems.reserve(size);
      for (auto i = 0; i < size; ++i) {
        is >> unmanagedData->interpArrayItems.emplace_back(version);
      }
    }
  }

  if (interpArrayItems) {
    // TODO: Visit
    if (arrayParams) {
      auto size = static_cast<std::size_t>(arrayParams.value<true>().arraySize);
      interpArrayItems->reserve(size);
      for (auto i = 0; i < size; ++i) {
        is >> interpArrayItems->emplace_back(version);
      }
    } else {
      auto size = arrayParams.value<false>().arraySize;
      interpArrayItems->reserve(size);
      for (auto i = 0; i < size; ++i) {
        is >> interpArrayItems->emplace_back(version);
      }
    }
  }

  io::readBytes(is, managerControlled);
  if (weightThresholdL) io::readBytes(is, *weightThresholdL);
  io::readBytes(is, onlyUseHeighestWeight);
  io::readBytes(is, interpCount);
  io::readBytes(is, singleIndex);
  is >> singleInterpolator;
  io::readBytes(is, singleTime);
  io::readBytes(is, highPriority);
  io::readBytes(is, nextHighPriority);
}

void BSXFlags::read(std::istream &is) {
  NiIntegerExtraData::read(is);
}

void NiSequence::read(std::istream &is) {
  NiObject::read(is);
  is >> name;
  is >> accumRootName;
  is >> textKeys;
  io::readBytes(is, numControlledBlocks);
  io::readBytes(is, arrayGrowBy);
  controlledBlocks.reserve(numControlledBlocks);
  for (auto i = 0; i < numControlledBlocks; ++i) {
    is >> controlledBlocks.emplace_back(version);
  }
}

void NiControllerSequence::read(std::istream &is) {
  NiSequence::read(is);
  io::readBytes(is, weight);
  is >> textKeys;
  io::readBytes(is, cycleType);
  io::readBytes(is, frequency);
  io::readBytes(is, phase);
  io::readBytes(is, startTime);
  io::readBytes(is, stopTime);
  io::readBytes(is, playBackwards);
  is >> manager;
  is >> accumRootName;
  is >> stringPalette;
}

void NiAVObjectPalette::read(std::istream &is) {
  NiObject::read(is);
}

void NiDefaultAVObjectPalette::read(std::istream &is) {
  NiAVObjectPalette::read(is);
  is >> scene;
  io::readBytes(is, numObjects);
  objects.assign(numObjects, {});
  for (auto &obj : objects) is >> obj;
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

void NiControllerManager::read(std::istream &is) {
  NiTimeController::read(is);
  io::readBytes(is, cumulative);
  io::readBytes(is, numControllerSequences);
  controllerSequences.assign(numControllerSequences, {});
  for (auto &seq : controllerSequences) is >> seq;
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
  is >> controller;
}

void NiProperty::read(std::istream &is) {
  NiObjectNet::read(is);
}

void NiMaterialProperty::read(std::istream &is) {
  NiProperty::read(is);
  io::readBytes(is, flags);
  is >> ambientColor;
  is >> diffuseColor;
  is >> specularColor;
  is >> emissiveColor;
  io::readBytes(is, glossiness);
  io::readBytes(is, alpha);
}

void NiTexturingProperty::read(std::istream &is) {
  NiProperty::read(is);
  io::readBytes(is, flags);
  io::readBytes(is, applyMode);
  io::readBytes(is, textureCount);

  io::readBytes(is, hasBaseTexture);
  if (hasBaseTexture) is >> baseTexture;

  io::readBytes(is, hasDarkTexture);
  if (hasDarkTexture) is >> darkTexture;

  io::readBytes(is, hasDetailTexture);
  if (hasDetailTexture) is >> detailTexture;

  io::readBytes(is, hasGlossTexture);
  if (hasGlossTexture) is >> glossTexture;

  io::readBytes(is, hasGlowTexture);
  if (hasGlowTexture) is >> glowTexture;

  if (textureCount > 5) {
    io::readBytes(is, hasBumpTexture);
    if (hasBumpTexture && *hasBumpTexture) {
      BumpInfo bumpInfo{version};
      is >> bumpInfo.bumpTexture;
      io::readBytes(is, bumpInfo.lumaScale);
      io::readBytes(is, bumpInfo.lumaOffset);
      is >> bumpInfo.matrix;
      bumpTextureData.emplace(bumpInfo);
    }
  }

  if (textureCount > 6) {
    io::readBytes(is, hasDecal0Texture);
    if (hasDecal0Texture) is >> decal0Texture;
  }

  if (textureCount > 7) {
    io::readBytes(is, hasDecal1Texture);
    if (hasDecal1Texture) is >> decal1Texture;
  }

  if (textureCount > 8) {
    io::readBytes(is, hasDecal2Texture);
    if (hasDecal2Texture) is >> decal2Texture;
  }

  if (textureCount > 9) {
    io::readBytes(is, hasDecal3Texture);
    if (hasDecal3Texture) is >> decal3Texture;
  }

  io::readBytes(is, numShaderTextures);
  if (numShaderTextures && shaderTextures) {
    for (auto i = 0; i < *numShaderTextures; ++i) {
      shaderTextures->emplace_back(version);
      is >> shaderTextures->back();
    }
  }
}

void NiStencilProperty::read(std::istream &is) {
  NiProperty::read(is);
  io::readBytes(is, flags);
  io::readBytes(is, stencilEnabled);
  io::readBytes(is, stencilFunction);
  io::readBytes(is, stencilRef);
  io::readBytes(is, stencilMask);
  io::readBytes(is, failAction);
  io::readBytes(is, zfailAction);
  io::readBytes(is, passAction);
  io::readBytes(is, drawMode);
}

void NiVertexColorProperty::read(std::istream &is) {
  NiProperty::read(is);
  io::readBytes(is, flags);
  io::readBytes(is, vertexMode);
  io::readBytes(is, lightMode);
}

void NiAlphaProperty::read(std::istream &is) {
  NiProperty::read(is);
  uint16_t flags{};
  io::readBytes(is, flags);
  alphaBlendingEnabled = (flags & 0b1) != 0;
  sourceBlendMode =
      BlendMode(static_cast<uint8_t>((flags & 0b1111'0) >> 1));
  destinationBlendMode =
      BlendMode(static_cast<uint8_t>((flags & 0b1111'0000'0) >> 5));
  alphaTestEnabled = (flags & 0b1'0000'0000'0) != 0;
  alphaTestMode =
      TestMode(static_cast<uint8_t>((flags & 0b111'0'0000'0000'0)) >> 10);
  disableTriangleSorting = (flags & 0b1'000'0'0000'0000'0) != 0;

  io::readBytes(is, threshold);
}

void NiSpecularProperty::read(std::istream &is) {
  NiProperty::read(is);
  io::readBytes(is, flags);
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
    if ((hasVertexWeights && *hasVertexWeights) || !hasVertexWeights) {
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
  is >> data;
  is >> skinPartition;
  is >> skeletonRoot;
  io::readBytes(is, numBones);
  bones.reserve(numBones);
  for (auto i = 0; i < numBones; ++i) {
    bones.emplace_back();
    is >> bones.back();
  }
}

void NiGeometry::read(std::istream &is) {
  NiAVObject::read(is);
  is >> data;
  is >> skinInstance;
  is >> materialData;
}

void NiTriBasedGeom::read(std::istream &is) {
  NiGeometry::read(is);
}

void NiTriShape::read(std::istream &is) {
  NiTriBasedGeom::read(is);
}

void NiTriStrips::read(std::istream &is) {
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
  if (hasVertexColors) {
    vertexColors.reserve(numVertices);
    for (auto i = 0; i < numVertices; ++i) {
      vertexColors.emplace_back();
      is >> vertexColors.back();
    }
  }

  io::readBytes(is, numUVSets);
  io::readBytes(is, hasUV);
  // First array dimension is the number of uv sets. For high versions this is
  // the lower 5 bits of vectorFlags, for low versions this is the lower 5 bits
  // of numUVSets. Since hasUV is present only if numUVSets is, and
  // hasUV == (numUVSets != 0), we can ignore hasUV.
  uint16_t arr1{};
  if (vectorFlags) {
    arr1 = static_cast<uint16_t>(*vectorFlags & Enum::VectorFlags::VF_UV_MASK);
  } else if (numUVSets) {
    arr1 = *numUVSets & 0b11111;
  }
  for (auto i = 0; i < arr1; ++i) {
    std::vector<compound::TexCoord> row{};
    for (auto j = 0; j < numVertices; ++j) {
      row.emplace_back();
      is >> row.back();
    }
    uvSets.push_back(row);
  }

  io::readBytes(is, consistencyFlags);
  is >> additionalData;
}

void NiTriBasedGeomData::read(std::istream &is) {
  NiGeometryData::read(is);
  io::readBytes(is, numTriangles);
}

void NiTriShapeData::read(std::istream &is) {
  NiTriBasedGeomData::read(is);

  io::readBytes(is, numTrianglePoints);
  io::readBytes(is, hasTriangles);
  triangles.reserve(numTriangles);
  for (basic::UShort i = 0; i < numTriangles; ++i) {
    triangles.emplace_back();
    is >> triangles.back();
  }

  io::readBytes(is, numMatchGroups);
  matchGroups.reserve(numMatchGroups);
  for (basic::UShort i = 0; i < numMatchGroups; ++i) {
    matchGroups.emplace_back();
    is >> matchGroups.back();
  }
}

void NiTriStripsData::read(std::istream &is) {
  NiTriBasedGeomData::read(is);
  io::readBytes(is, numStrips);
  io::readBytes(is, stripLengths, numStrips);

  io::readBytes(is, hasPoints);
  if ((hasPoints && *hasPoints) || !hasPoints) {
    points.reserve(numStrips);
    for (auto i = 0; i < numStrips; ++i) {
      points.emplace_back();
      io::readBytes(is, points.back(), stripLengths[i]);
    }
  }
}

void NiTexture::read(std::istream &is) {
  NiObjectNet::read(is);
}

void NiSourceTexture::read(std::istream &is) {
  NiTexture::read(is);

  io::readBytes(is, useExternal);

  if (useExternal) {
    ExternalTextureFile &texFile =
        textureFileData.emplace<ExternalTextureFile>(version);
    is >> texFile.filename;
    is >> texFile.unknownRef;
  } else {
    InternalTextureFile &texFile =
        textureFileData.emplace<InternalTextureFile>(version);
    io::readBytes(is, texFile.unknownByte);
    is >> texFile.filename;
  }

  is >> formatPrefs;
  io::readBytes(is, isStatic);
  io::readBytes(is, directRender);
}

} // namespace nif
