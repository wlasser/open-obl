#include "nif/compound.hpp"
#include <variant>

namespace nif::compound {

std::string SizedString::str() const {
  return std::string(value.begin(), value.end());
}

std::istream &operator>>(std::istream &is, SizedString &t) {
  io::readBytes(is, t.length);
  io::readBytes(is, t.value, t.length);
  return is;
}

std::istream &operator>>(std::istream &is, StringPalette &t) {
  is >> t.palette;
  io::readBytes(is, t.length);
  return is;
}

std::istream &operator>>(std::istream &is, ByteArray &t) {
  io::readBytes(is, t.dataSize);
  io::readBytes(is, t.data, t.dataSize);
  return is;
}

std::istream &operator>>(std::istream &is, ByteMatrix &t) {
  io::readBytes(is, t.dataSize1);
  io::readBytes(is, t.dataSize2);

  // It is unclear from the niftools documentation which ordering is used for
  // arrays, but pyffi/object_models/xml/array.py#08f21fa seems to suggest
  // nesting arr2 in arr1.
  for (basic::UInt i = 0; i < t.dataSize2; ++i) {
    std::vector<basic::Byte> row{};
    io::readBytes(is, row, t.dataSize1);
    t.data.push_back(row);
  }

  return is;
}

std::istream &operator>>(std::istream &is, FilePath &t) {
  is >> t.string;
  return is;
}

std::istream &operator>>(std::istream &is, LODRange &t) {
  io::readBytes(is, t.nearExtent);
  io::readBytes(is, t.farExtent);
  io::readBytes(is, t.unknown);
  return is;
}

std::istream &operator>>(std::istream &is, MatchGroup &t) {
  io::readBytes(is, t.numVertices);
  io::readBytes(is, t.vertexIndices, t.numVertices);
  return is;
}

std::istream &operator>>(std::istream &is, Quaternion &t) {
  io::readBytes(is, t.w);
  io::readBytes(is, t.x);
  io::readBytes(is, t.y);
  io::readBytes(is, t.z);
  return is;
}

std::istream &operator>>(std::istream &is, Matrix22 &t) {
  io::readBytes(is, t);
  return is;
}

std::istream &operator>>(std::istream &is, Matrix33 &t) {
  io::readBytes(is, t);
  return is;
}

std::istream &operator>>(std::istream &is, Matrix34 &t) {
  io::readBytes(is, t);
  return is;
}

std::istream &operator>>(std::istream &is, Matrix44 &t) {
  io::readBytes(is, t);
  return is;
}

std::istream &operator>>(std::istream &is, MipMap &t) {
  io::readBytes(is, t.width);
  io::readBytes(is, t.height);
  io::readBytes(is, t.offset);
  return is;
}

std::istream &operator>>(std::istream &is, ShortString &t) {
  io::readBytes(is, t.length);
  io::readBytes(is, t.value, static_cast<std::size_t>(t.length));
  return is;
}

std::istream &operator>>(std::istream &is, ExportInfo &t) {
  is >> t.author;
  is >> t.processScript;
  is >> t.exportScript;
  return is;
}

std::istream &operator>>(std::istream &is, NiPlane &t) {
  is >> t.normal;
  io::readBytes(is, t.constant);
  return is;
}

std::istream &operator>>(std::istream &is, NiBound &t) {
  is >> t.center;
  io::readBytes(is, t.radius);
  return is;
}

std::istream &operator>>(std::istream &is, BoxBV &t) {
  is >> t.center;
  for (auto &ax : t.axis) {
    is >> ax;
  }
  is >> t.extent;
  return is;
}

std::istream &operator>>(std::istream &is, CapsuleBV &t) {
  is >> t.center;
  is >> t.origin;
  io::readBytes(is, t.extent);
  io::readBytes(is, t.radius);
  return is;
}

std::istream &operator>>(std::istream &is, UnionBV &t) {
  io::readBytes(is, t.numBoundingVolumes);
  t.boundingVolumes.reserve(t.numBoundingVolumes);
  for (basic::UInt i = 0u; i < t.numBoundingVolumes; ++i) {
    t.boundingVolumes.emplace_back<BoundingVolume>({});
    is >> t.boundingVolumes.back();
  }
  return is;
}

std::istream &operator>>(std::istream &is, HalfSpaceBV &t) {
  is >> t.plane;
  is >> t.center;
  return is;
}

std::istream &operator>>(std::istream &is, EmptyBV &) {
  return is;
}

std::istream &operator>>(std::istream &is, BoundingVolume &t) {
  io::readBytes(is, t.collisionType);
  switch (t.collisionType) {
    case Enum::BoundVolumeType::BASE_BV:t.collision = NiBound{};
      is >> std::get<0>(t.collision);
      break;
    case Enum::BoundVolumeType::BOX_BV:t.collision = BoxBV{};
      is >> std::get<1>(t.collision);
      break;
    case Enum::BoundVolumeType::CAPSULE_BV:t.collision = CapsuleBV{};
      is >> std::get<2>(t.collision);
      break;
    case Enum::BoundVolumeType::UNION_BV:t.collision = UnionBV{};
      is >> std::get<4>(t.collision);
      break;
    case Enum::BoundVolumeType::HALFSPACE_BV:t.collision = HalfSpaceBV{};
      is >> std::get<5>(t.collision);
      break;
    default: break;
  }
  return is;
}

std::istream &operator>>(std::istream &is, Header &t) {
  is >> t.headerString;
  if (t.copyright) {
    for (int i = 0; i < 3; ++i) {
      basic::LineString s{};
      is >> s;
      t.copyright->operator[](i) = s;
    }
  }
  io::readBytes(is, t.ver);
  io::readBytes(is, t.endianType);
  io::readBytes(is, t.userVer);
  io::readBytes(is, t.numBlocks);

  if (t.mVersion == "10.0.1.2"_ver ||
      (t.mVersion == "20.2.0.7"_ver || t.mVersion == "20.0.0.5"_ver ||
          ("10.1.0.0"_ver <= t.mVersion && t.mVersion <= "20.0.0.4"_ver
              && t.userVer && t.userVer.value() <= 11))) {
    io::readBytes(is, t.bsStreamHeader.userVersion2);
    is >> t.bsStreamHeader.exportInfo;
    if (t.bsStreamHeader.userVersion2 == 130) {
      is >> t.bsStreamHeader.maxFilepath;
    }
  }
  is >> t.metadata;
  io::readBytes(is, t.numBlockTypes);

  if (t.blockTypes) {
    t.blockTypes->reserve(t.numBlockTypes.value());
    for (basic::UShort i = 0; i < t.numBlockTypes.value(); ++i) {
      SizedString s{};
      is >> s;
      t.blockTypes->push_back(s);
    }
  }

  if (t.blockTypeIndices) {
    io::readBytes(is, t.blockTypeIndices.value(), t.numBlocks.value());
  }

  if (t.blockSizes) {
    io::readBytes(is, t.blockSizes.value(), t.numBlocks.value());
  }

  io::readBytes(is, t.numStrings);
  io::readBytes(is, t.maxStringLength);

  if (t.strings) {
    t.strings->reserve(t.numStrings.value());
    for (basic::UInt i = 0; i < t.numStrings.value(); ++i) {
      SizedString s{};
      is >> s;
      t.strings->push_back(s);
    }
  }

  io::readBytes(is, t.numGroups);

  if (t.groups) {
    io::readBytes(is, t.groups.value(), t.numGroups.value());
  }

  return is;
}

std::istream &operator>>(std::istream &is, MaterialData &t) {
  io::readBytes(is, t.hasShader);
  if (t.hasShader) {
    is >> t.shaderName;
    io::readBytes(is, t.shaderExtraData);
  }

  return is;
}

std::istream &operator>>(std::istream &is, Triangle &t) {
  io::readBytes(is, t.v1);
  io::readBytes(is, t.v2);
  io::readBytes(is, t.v3);

  return is;
}

std::istream &operator>>(std::istream &is, SkinPartition &t) {
  io::readBytes(is, t.numVertices);
  io::readBytes(is, t.numTriangles);
  io::readBytes(is, t.numBones);
  io::readBytes(is, t.numStrips);
  io::readBytes(is, t.numWeightsPerVertex);
  io::readBytes(is, t.bones, t.numBones);

  io::readBytes(is, t.hasVertexMap);
  if ((t.hasVertexMap && *t.hasVertexMap) || !t.hasVertexMap) {
    io::readBytes(is, t.vertexMap, t.numVertices);
  }

  io::readBytes(is, t.hasVertexWeights);
  if ((t.hasVertexWeights && *t.hasVertexWeights) || !t.hasVertexWeights) {
    for (auto i = 0; i < t.numVertices; ++i) {
      t.vertexWeights.emplace_back();
      io::readBytes(is, t.vertexWeights.back(), t.numWeightsPerVertex);
    }
  }

  io::readBytes(is, t.stripLengths, t.numStrips);

  io::readBytes(is, t.hasFaces);
  if ((t.hasFaces && *t.hasFaces) || !t.hasFaces) {
    if (t.numStrips == 0) {
      t.triangles.reserve(t.numTriangles);
      for (auto i = 0; i < t.numTriangles; ++i) {
        t.triangles.emplace_back();
        is >> t.triangles.back();
      }
    } else {
      for (auto i = 0; i < t.numStrips; ++i) {
        t.strips.emplace_back();
        io::readBytes(is, t.strips.back(), t.stripLengths[i]);
      }
    }
  }

  io::readBytes(is, t.hasBoneIndices);
  for (auto i = 0; i < t.numVertices; ++i) {
    t.boneIndices.emplace_back();
    io::readBytes(is, t.boneIndices.back(), t.numWeightsPerVertex);
  }

  return is;
}

std::istream &operator>>(std::istream &is, BoneVertData &t) {
  io::readBytes(is, t.index);
  io::readBytes(is, t.weight);

  return is;
}

std::istream &operator>>(std::istream &is, NiTransform &t) {
  is >> t.rotation;
  is >> t.translation;
  io::readBytes(is, t.scale);

  return is;
}

std::istream &operator>>(std::istream &is, NiQuatTransform &t) {
  is >> t.translation;
  is >> t.rotation;
  io::readBytes(is, t.scale);
  io::readBytes(is, t.trsValid);

  return is;
}

std::istream &operator>>(std::istream &is, HavokFilter &t) {
  io::readBytes(is, t.layer);
  uint8_t flagsAndPart{0};
  io::readBytes(is, flagsAndPart);
  t.flags = HavokFilter::Flags(flagsAndPart & 0b11100000u);
  t.part = HavokFilter::Part(flagsAndPart & 0b00011111u);
  io::readBytes(is, t.group);

  return is;
}

std::istream &operator>>(std::istream &is, HavokMaterial &t) {
  io::readBytes(is, t.unknown);
  io::readBytes(is, t.material);

  return is;
}

std::istream &operator>>(std::istream &is, TexCoord &t) {
  io::readBytes(is, t.u);
  io::readBytes(is, t.v);

  return is;
}

std::istream &operator>>(std::istream &is, TexDesc &t) {
  is >> t.source;
  io::readBytes(is, t.clampMode);
  io::readBytes(is, t.filterMode);
  io::readBytes(is, t.uvSet);
  io::readBytes(is, t.ps2L);
  io::readBytes(is, t.ps2K);
  io::readBytes(is, t.unknown);
  io::readBytes(is, t.hasTextureTransform);

  if (t.hasTextureTransform && *t.hasTextureTransform) {
    TexDesc::NiTextureTransform trans{};
    is >> trans.translation >> trans.scale;
    io::readBytes(is, trans.rotation);
    io::readBytes(is, trans.transformMethod);
    is >> trans.center;
    t.textureTransform = trans;
  }
  return is;
}

std::istream &operator>>(std::istream &is, ShaderTexDesc &t) {
  io::readBytes(is, t.hasMap);
  if (t.hasMap) {
    ShaderTexDesc::Map map{t.mVersion};
    is >> map.map;
    io::readBytes(is, map.mapID);
    t.map.emplace(map);
  }
  return is;
}

std::istream &operator>>(std::istream &is, AdditionalDataInfo &t) {
  io::readBytes(is, t.dataType);
  io::readBytes(is, t.numChannelBytesPerElement);
  io::readBytes(is, t.numChannelBytes);
  io::readBytes(is, t.numTotalBytesPerElement);
  io::readBytes(is, t.blockIndex);
  io::readBytes(is, t.channelOffset);
  io::readBytes(is, t.unknown);

  return is;
}

std::istream &operator>>(std::istream &is, AdditionalDataBlock &t) {
  io::readBytes(is, t.hasData);
  io::readBytes(is, t.blockSize);
  io::readBytes(is, t.numBlocks);
  io::readBytes(is, t.blockOffsets, static_cast<std::size_t>(t.numBlocks));
  io::readBytes(is, t.numData);
  io::readBytes(is, t.dataSizes, static_cast<std::size_t>(t.numData));
  for (auto i = 0; i < t.numData; ++i) {
    std::vector<basic::Byte> row{};
    io::readBytes(is, row, static_cast<std::size_t>(t.blockSize));
    t.data.push_back(row);
  }
  return is;
}

std::istream &operator>>(std::istream &is, FormatPrefs &t) {
  io::readBytes(is, t.pixelLayout);
  io::readBytes(is, t.mipMapFormat);
  io::readBytes(is, t.alphaFormat);

  return is;
}

std::istream &operator>>(std::istream &is, TriangleData &t) {
  is >> t.triangle;
  io::readBytes(is, t.weldingInfo);
  is >> t.normal;

  return is;
}

std::istream &operator>>(std::istream &is, OblivionSubShape &t) {
  is >> t.havokFilter;
  io::readBytes(is, t.numVertices);
  is >> t.havokMaterial;

  return is;
}

std::istream &operator>>(std::istream &is, TBC &t) {
  io::readBytes(is, t.tension);
  io::readBytes(is, t.bias);
  io::readBytes(is, t.continuity);

  return is;
}

std::istream &operator>>(std::istream &is,
                         Key<Quaternion, Enum::KeyType::TBC_KEY> &t) {
  io::readBytes(is, t.time);
  is >> t.value;
  is >> t.tbc;
  return is;
}

std::istream &operator>>(std::istream &is, ControlledBlock &t) {
  is >> t.targetName;
  is >> t.interpolator;
  is >> t.controller;
  is >> t.blendInterpolator;
  io::readBytes(is, t.blendIndex);
  io::readBytes(is, t.priority);
  if (t.idTag) {
    is >> t.idTag->nodeName;
    is >> t.idTag->propertyType;
    is >> t.idTag->controllerType;
    is >> t.idTag->controllerID;
    is >> t.idTag->interpolatorID;
  }
  if (t.palette) {
    is >> t.palette->stringPalette;
    io::readBytes(is, t.palette->nodeNameOffset);
    io::readBytes(is, t.palette->propertyTypeOffset);
    io::readBytes(is, t.palette->controllerTypeOffset);
    io::readBytes(is, t.palette->controllerIDOffset);
    io::readBytes(is, t.palette->interpolatorIDOffset);
  }

  return is;
}

std::istream &operator>>(std::istream &is, AVObject &t) {
  is >> t.name;
  is >> t.avObject;

  return is;
}

std::istream &operator>>(std::istream &is, InterpBlendItem &t) {
  is >> t.interpolator;
  io::readBytes(is, t.weight);
  io::readBytes(is, t.normalizedWeight);
  io::readBytes(is, t.priority);
  io::readBytes(is, t.easeSpinner);

  return is;
}

std::istream &operator>>(std::istream &is, FurniturePosition &t) {
  is >> t.offset;
  io::readBytes(is, t.orientation);
  io::readBytes(is, t.positionRef1);
  io::readBytes(is, t.positionRef2);

  return is;
}

std::istream &operator>>(std::istream &is, BallAndSocketDescriptor &t) {
  is >> t.pivotA;
  is >> t.pivotB;

  return is;
}

std::istream &operator>>(std::istream &is, HingeDescriptor &t) {
  is >> t.pivotA;
  is >> t.perpAxisInA1;
  is >> t.perpAxisInA2;
  is >> t.pivotB;
  is >> t.axisB;

  return is;
}

std::istream &operator>>(std::istream &is, LimitedHingeDescriptor &t) {
  is >> t.pivotA;
  is >> t.axisA;
  is >> t.perpAxisInA1;
  is >> t.perpAxisInA2;

  is >> t.pivotB;
  is >> t.axisB;
  is >> t.perpAxisInB2;

  io::readBytes(is, t.minAngle);
  io::readBytes(is, t.maxAngle);
  io::readBytes(is, t.maxFriction);

  return is;
}

std::istream &operator>>(std::istream &is, PrismaticDescriptor &t) {
  is >> t.pivotA;
  is >> t.rotationA;
  is >> t.planeA;
  is >> t.slidingA;

  is >> t.slidingB;
  is >> t.pivotB;
  is >> t.rotationB;
  is >> t.planeB;

  io::readBytes(is, t.minDistance);
  io::readBytes(is, t.maxDistance);
  io::readBytes(is, t.friction);
  return is;
}

std::istream &operator>>(std::istream &is, RagdollDescriptor &t) {
  is >> t.pivotA;
  is >> t.planeA;
  is >> t.twistA;

  is >> t.pivotB;
  is >> t.planeB;
  is >> t.twistB;

  io::readBytes(is, t.coneMaxAngle);
  io::readBytes(is, t.planeMinAngle);
  io::readBytes(is, t.planeMaxAngle);
  io::readBytes(is, t.twistMinAngle);
  io::readBytes(is, t.twistMaxAngle);
  io::readBytes(is, t.maxFriction);

  return is;
}

std::istream &operator>>(std::istream &is, StiffSpringDescriptor &t) {
  is >> t.pivotA;
  is >> t.pivotB;
  io::readBytes(is, t.length);

  return is;
}

std::istream &operator>>(std::istream &is, MalleableDescriptor &t) {
  io::readBytes(is, t.constraintType);
  io::readBytes(is, t.numEntities);

  is >> t.entityA;
  is >> t.entityB;

  io::readBytes(is, t.priority);

  using CT = Enum::hk::ConstraintType;

  switch (t.constraintType) {
    default: [[fallthrough]];
    case CT::BallAndSocket:is >> t.descriptor.emplace<0>();
      break;
    case CT::Hinge:is >> t.descriptor.emplace<1>();
      break;
    case CT::LimitedHinge:is >> t.descriptor.emplace<2>();
      break;
    case CT::Prismatic: is >> t.descriptor.emplace<3>();
      break;
    case CT::Ragdoll: is >> t.descriptor.emplace<4>();
      break;
    case CT::StiffSpring: is >> t.descriptor.emplace<5>();
      break;
  }

  io::readBytes(is, t.tau);
  io::readBytes(is, t.damping);

  return is;
}

std::istream &operator>>(std::istream &is, ConstraintData &t) {
  io::readBytes(is, t.constraintType);
  io::readBytes(is, t.numEntities);

  is >> t.entityA;
  is >> t.entityB;

  io::readBytes(is, t.priority);

  using CT = Enum::hk::ConstraintType;

  switch (t.constraintType) {
    default: [[fallthrough]];
    case CT::BallAndSocket:is >> t.descriptor.emplace<0>();
      break;
    case CT::Hinge:is >> t.descriptor.emplace<1>();
      break;
    case CT::LimitedHinge:is >> t.descriptor.emplace<2>();
      break;
    case CT::Prismatic: is >> t.descriptor.emplace<3>();
      break;
    case CT::Ragdoll: is >> t.descriptor.emplace<4>();
      break;
    case CT::StiffSpring: is >> t.descriptor.emplace<5>();
      break;
    case CT::Malleable: is >> t.descriptor.emplace<6>();
      break;
  }

  return is;
}

namespace hk {

std::istream &operator>>(std::istream &is, Quaternion &t) {
  io::readBytes(is, t.x);
  io::readBytes(is, t.y);
  io::readBytes(is, t.z);
  io::readBytes(is, t.w);
  return is;
}

std::istream &operator>>(std::istream &is, Matrix3 &t) {
  io::readBytes(is, t);
  return is;
}

std::istream &operator>>(std::istream &is, WorldObjCinfoProperty &t) {
  io::readBytes(is, t.data);
  io::readBytes(is, t.size);
  io::readBytes(is, t.capacityAndFlags);

  return is;
}

} // namespace hk

} // namespace nif::compound