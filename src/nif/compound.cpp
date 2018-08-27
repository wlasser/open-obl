#include "nif/compound.hpp"
#include <variant>

namespace nif::compound {

std::istream &operator>>(std::istream &is, SizedString &t) {
  io::readBytes(is, t.length);
  io::readBytes(is, t.value, t.length);
  return is;
}

std::istream &operator>>(std::istream &is, String &t) {
  is >> t.string;
  io::readBytes(is, t.index);
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
  io::readBytes(is, t.index);
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

std::istream &operator>>(std::istream &is, hkQuaternion &t) {
  io::readBytes(is, t.x);
  io::readBytes(is, t.y);
  io::readBytes(is, t.z);
  io::readBytes(is, t.w);
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

std::istream &operator>>(std::istream &is, hkMatrix3 &t) {
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
  for (int i = 0; i < 3; ++i) is >> t.axis[i];
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
  for (auto i = 0; i < t.numBoundingVolumes; ++i) {
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

std::istream &operator>>(std::istream &is, EmptyBV &t) {
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

  if (t.version == "10.0.1.2"_ver ||
      (t.version == "20.2.0.7"_ver || t.version == "20.0.0.5"_ver ||
          ("10.1.0.0"_ver <= t.version && t.version <= "20.0.0.4"_ver
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
  is >> t.shaderName;
  io::readBytes(is, t.shaderExtraData);

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
      std::vector<basic::Float> row{};
      io::readBytes(is, row, t.numWeightsPerVertex);
      t.vertexWeights.push_back(row);
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
        std::vector<basic::UShort> row{};
        io::readBytes(is, row, t.stripLengths[i]);
        t.strips.push_back(row);
      }
    }
  }

  io::readBytes(is, t.hasBoneIndices);
  for (auto i = 0; i < t.numVertices; ++i) {
    std::vector<basic::Byte> row{};
    io::readBytes(is, row, t.numWeightsPerVertex);
    t.boneIndices.push_back(row);
  }

  return is;
}

std::istream &operator>>(std::istream &is, BoneVertData &t) {
  io::readBytes(is, t.index);
  io::readBytes(is, t.weight);

  return is;
}

std::istream &operator>>(std::istream &is, NiTransform &t) {
  io::readBytes(is, t.rotation);
  io::readBytes(is, t.translation);
  io::readBytes(is, t.scale);

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
    ShaderTexDesc::Map map{t.version};
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

} // namespace nif::compound