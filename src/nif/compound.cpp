#include "nif/compound.hpp"

std::istream &nif::compound::operator>>(std::istream &is, SizedString &t) {
  io::readBytes(is, t.length);
  io::readBytes(is, t.value, t.length);
  return is;
}

std::istream &nif::compound::operator>>(std::istream &is, String &t) {
  is >> t.string;
  io::readBytes(is, t.index);
  return is;
}

std::istream &nif::compound::operator>>(std::istream &is, ByteArray &t) {
  io::readBytes(is, t.dataSize);
  io::readBytes(is, t.data, t.dataSize);
  return is;
}

std::istream &nif::compound::operator>>(std::istream &is, ByteMatrix &t) {
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

std::istream &nif::compound::operator>>(std::istream &is, FilePath &t) {
  is >> t.string;
  io::readBytes(is, t.index);
  return is;
}

std::istream &nif::compound::operator>>(std::istream &is, LODRange &t) {
  io::readBytes(is, t.nearExtent);
  io::readBytes(is, t.farExtent);
  io::readBytes(is, t.unknown);
  return is;
}

std::istream &nif::compound::operator>>(std::istream &is, MatchGroup &t) {
  io::readBytes(is, t.numVertices);
  io::readBytes(is, t.vertexIndices, t.numVertices);
  return is;
}

std::istream &nif::compound::operator>>(std::istream &is, Quaternion &t) {
  io::readBytes(is, t.w);
  io::readBytes(is, t.x);
  io::readBytes(is, t.y);
  io::readBytes(is, t.z);
  return is;
}

std::istream &nif::compound::operator>>(std::istream &is, hkQuaternion &t) {
  io::readBytes(is, t.x);
  io::readBytes(is, t.y);
  io::readBytes(is, t.z);
  io::readBytes(is, t.w);
  return is;
}

std::istream &nif::compound::operator>>(std::istream &is, Matrix22 &t) {
  io::readBytes(is, t);
  return is;
}

std::istream &nif::compound::operator>>(std::istream &is, Matrix33 &t) {
  io::readBytes(is, t);
  return is;
}

std::istream &nif::compound::operator>>(std::istream &is, Matrix34 &t) {
  io::readBytes(is, t);
  return is;
}

std::istream &nif::compound::operator>>(std::istream &is, Matrix44 &t) {
  io::readBytes(is, t);
  return is;
}

std::istream &nif::compound::operator>>(std::istream &is, hkMatrix3 &t) {
  io::readBytes(is, t);
  return is;
}

std::istream &nif::compound::operator>>(std::istream &is, MipMap &t) {
  io::readBytes(is, t.width);
  io::readBytes(is, t.height);
  io::readBytes(is, t.offset);
  return is;
}

std::istream &nif::compound::operator>>(std::istream &is, ShortString &t) {
  io::readBytes(is, t.length);
  io::readBytes(is, t.value, static_cast<std::size_t>(t.length));
  return is;
}

std::istream &nif::compound::operator>>(std::istream &is, ExportInfo &t) {
  is >> t.author;
  is >> t.processScript;
  is >> t.exportScript;
  return is;
}

std::istream &nif::compound::operator>>(std::istream &is, Header &t) {
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
