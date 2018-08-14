#ifndef OPENOBLIVION_NIF_COMPOUND_HPP
#define OPENOBLIVION_NIF_COMPOUND_HPP

#include "io/read_bytes.hpp"
#include "nif/basic.hpp"
#include "nif/enum.hpp"
#include "nif/versionable.hpp"
#include <array>
#include <vector>

namespace nif {
namespace compound {

using namespace nif::literals;

// BString
struct SizedString {
  basic::UInt length{};
  std::vector<basic::Char> value{};
};
std::istream &operator>>(std::istream &is, SizedString &t) {
  io::readBytes(is, t.length);
  for (basic::UInt i = 0; i < t.length; ++i) {
    basic::Char c{};
    io::readBytes(is, c);
    t.value.push_back(c);
  }
  return is;
}

struct String : Versionable {
  VersionOptional<SizedString, Unbounded, "20.0.0.5"_ver> string{version};
  VersionOptional<basic::StringIndex, "20.1.0.3"_ver, Unbounded> index{version};
  explicit String(uint32_t version) : Versionable(version) {}
};
std::istream &operator>>(std::istream &is, String &t) {
  is >> t.string;
  io::readBytes(is, t.index);
  return is;
}

struct ByteArray {
  basic::UInt dataSize{};
  std::vector<basic::Byte> data{};
};
std::istream &operator>>(std::istream &is, ByteArray &t) {
  io::readBytes(is, t.dataSize);
  for (basic::UInt i = 0; i < t.dataSize; ++i) {
    basic::Byte b{};
    io::readBytes(is, b);
    t.data.push_back(b);
  }
  return is;
}

struct ByteMatrix {
  basic::UInt dataSize1{};
  basic::UInt dataSize2{};
  // arr1 = dataSize2, arr2 = dataSize1
  std::vector<std::vector<basic::Byte>> data{};
};
std::istream &operator>>(std::istream &is, ByteMatrix &t) {
  io::readBytes(is, t.dataSize1);
  io::readBytes(is, t.dataSize2);

  // It is unclear from the niftools documentation which ordering is used for
  // arrays, but pyffi/object_models/xml/array.py#08f21fa seems to suggest
  // nesting arr2 in arr1.
  for (basic::UInt i = 0; i < t.dataSize2; ++i) {
    std::vector<basic::Byte> row{};
    row.reserve(t.dataSize1);
    for (basic::UInt j = 0; j < t.dataSize1; ++j) {
      basic::Byte b{};
      io::readBytes(is, b);
      row.push_back(b);
    }
    t.data.push_back(row);
  }

  return is;
}

template<class T>
struct Color3T {
  T r;
  T g;
  T b;
};
template<class T>
std::istream &operator>>(std::istream &is, Color3T<T> &t) {
  io::readBytes(is, t.r);
  io::readBytes(is, t.g);
  io::readBytes(is, t.b);
  return is;
}

template<class T>
struct Color4T {
  T r;
  T g;
  T b;
  T a;
};
template<class T>
std::istream &operator>>(std::istream &is, Color4T<T> &t) {
  io::readBytes(is, t.r);
  io::readBytes(is, t.g);
  io::readBytes(is, t.b);
  io::readBytes(is, t.a);
  return is;
}

using Color3 = Color3T<basic::Float>;
using ByteColor3 = Color3T<basic::Byte>;
using Color4 = Color4T<basic::Float>;
using ByteColor4 = Color4T<basic::Byte>;

struct FilePath : Versionable {
  VersionOptional<SizedString, Unbounded, "20.0.0.5"_ver> string{version};
  VersionOptional<basic::StringIndex, "20.1.0.3"_ver, Unbounded> index{version};
  explicit FilePath(uint32_t version) : Versionable(version) {}
};
std::istream &operator>>(std::istream &is, FilePath &t) {
  is >> t.string;
  io::readBytes(is, t.index);
  return is;
}

// TODO: This looks suspicious; is roots really a homogeneous array of actual
// NiObject types, or is it an array of objects deriving from NiObject?
template<class NiObject>
struct Footer : Versionable {
  // Number of root references
  VersionOptional<basic::UInt, "3.3.0.13"_ver, Unbounded> numRoots{version};
  // List of root nif objects. If there is a camera for 1st person view then it
  // is included in this list even if it is not a root object.
  VersionOptional<std::vector<NiObject>, "3.3.0.13"_ver, Unbounded>
      roots{version};
  explicit Footer(uint32_t version) : Versionable(version) {}
};
template<class NiObject>
std::istream &operator>>(std::istream &is, Footer<NiObject> &t) {
  io::readBytes(is, t.numRoots);
  if (t.roots) {
    t.roots->reserve(t.numRoots);
    for (basic::UInt i = 0; i < t.numRoots; ++i) {
      NiObject o{};
      is >> o;
      t.roots->push_back(o);
    }
  }
  return is;
}

struct LODRange {
  // Beginning of range
  basic::Float nearExtent{};
  // End of range
  basic::Float farExtent{};
  // Unknown
  std::array<basic::UInt, 3> unknown{};
};
std::istream &operator>>(std::istream &is, LODRange &t) {
  io::readBytes(is, t.nearExtent);
  io::readBytes(is, t.farExtent);
  io::readBytes(is, t.unknown);
  return is;
}

struct MatchGroup {
  basic::UShort numVertices{};
  std::vector<basic::UShort> vertexIndices{};
};
std::istream &operator>>(std::istream &is, MatchGroup &t) {
  io::readBytes(is, t.numVertices);
  t.vertexIndices.reserve(t.numVertices);
  for (basic::UShort i = 0; i < t.numVertices; ++i) {
    basic::UShort u{};
    io::readBytes(is, u);
    t.vertexIndices.push_back(u);
  }
  return is;
}

template<class T>
struct Vector3T {
  T x;
  T y;
  T z;
};
template<class T>
std::istream &operator>>(std::istream &is, Vector3T<T> &t) {
  io::readBytes(is, t.x);
  io::readBytes(is, t.y);
  io::readBytes(is, t.z);
  return is;
}

template<class T>
struct Vector4T {
  T x;
  T y;
  T z;
  T w;
};
template<class T>
std::istream &operator>>(std::istream &is, Vector4T<T> &t) {
  io::readBytes(is, t.x);
  io::readBytes(is, t.y);
  io::readBytes(is, t.z);
  io::readBytes(is, t.w);
  return is;
}

using ByteVector3 = Vector3T<basic::Byte>;
using Vector3 = Vector3T<basic::Float>;
using Vector4 = Vector4T<basic::Float>;

struct Quaternion {
  basic::Float w = 1.0f;
  basic::Float x = 0.0f;
  basic::Float y = 0.0f;
  basic::Float z = 0.0f;
};
std::istream &operator>>(std::istream &is, Quaternion &t) {
  io::readBytes(is, t.w);
  io::readBytes(is, t.x);
  io::readBytes(is, t.y);
  io::readBytes(is, t.z);
  return is;
}

struct hkQuaternion {
  basic::Float x = 0.0f;
  basic::Float y = 0.0f;
  basic::Float z = 0.0f;
  basic::Float w = 1.0f;
};
std::istream &operator>>(std::istream &is, hkQuaternion &t) {
  io::readBytes(is, t.x);
  io::readBytes(is, t.y);
  io::readBytes(is, t.z);
  io::readBytes(is, t.w);
  return is;
}

// Column major
struct Matrix22 {
  basic::Float m11 = 1.0f;
  basic::Float m21 = 0.0f;
  basic::Float m12 = 0.0f;
  basic::Float m22 = 1.0f;
};
std::istream &operator>>(std::istream &is, Matrix22 &t) {
  io::readBytes(is, t);
  return is;
}
// Column major
struct Matrix33 {
  basic::Float m11 = 1.0f;
  basic::Float m21 = 0.0f;
  basic::Float m31 = 0.0f;
  basic::Float m12 = 0.0f;
  basic::Float m22 = 1.0f;
  basic::Float m32 = 0.0f;
  basic::Float m13 = 0.0f;
  basic::Float m23 = 0.0f;
  basic::Float m33 = 1.0f;
};
std::istream &operator>>(std::istream &is, Matrix33 &t) {
  io::readBytes(is, t);
  return is;
}

// Column major
struct Matrix34 {
  basic::Float m11 = 1.0f;
  basic::Float m21 = 0.0f;
  basic::Float m31 = 0.0f;
  basic::Float m12 = 0.0f;
  basic::Float m22 = 1.0f;
  basic::Float m32 = 0.0f;
  basic::Float m13 = 0.0f;
  basic::Float m23 = 0.0f;
  basic::Float m33 = 1.0f;
  basic::Float m14 = 0.0f;
  basic::Float m24 = 0.0f;
  basic::Float m34 = 0.0f;
};
std::istream &operator>>(std::istream &is, Matrix34 &t) {
  io::readBytes(is, t);
  return is;
}

// Column major {
struct Matrix44 {
  basic::Float m11 = 1.0f;
  basic::Float m21 = 0.0f;
  basic::Float m31 = 0.0f;
  basic::Float m41 = 0.0f;
  basic::Float m12 = 0.0f;
  basic::Float m22 = 1.0f;
  basic::Float m32 = 0.0f;
  basic::Float m42 = 0.0f;
  basic::Float m13 = 0.0f;
  basic::Float m23 = 0.0f;
  basic::Float m33 = 1.0f;
  basic::Float m43 = 0.0f;
  basic::Float m14 = 0.0f;
  basic::Float m24 = 0.0f;
  basic::Float m34 = 0.0f;
  basic::Float m44 = 1.0f;
};
std::istream &operator>>(std::istream &is, Matrix44 &t) {
  io::readBytes(is, t);
  return is;
}

struct hkMatrix3 {
  basic::Float m11 = 1.0f;
  basic::Float m12 = 0.0f;
  basic::Float m13 = 0.0f;
  basic::Float m14{}; // Unused
  basic::Float m21 = 1.0f;
  basic::Float m22 = 0.0f;
  basic::Float m23 = 0.0f;
  basic::Float m24{}; // Unused
  basic::Float m31 = 1.0f;
  basic::Float m32 = 0.0f;
  basic::Float m33 = 0.0f;
  basic::Float m34{}; // Unused
};
std::istream &operator>>(std::istream &is, hkMatrix3 &t) {
  io::readBytes(is, t);
  return is;
}

struct MipMap {
  basic::UInt width{};
  basic::UInt height{};
  // Offset into the pixel data array where this mipmap starts.
  basic::UInt offset{};
};
std::istream &operator>>(std::istream &is, MipMap &t) {
  io::readBytes(is, t.width);
  io::readBytes(is, t.height);
  io::readBytes(is, t.offset);
  return is;
}

// TODO: Not sure if pointers to concrete NiNode, or polymorphic
template<class NiNode>
struct NodeSet {
  basic::UInt numNodes{};
  std::vector<basic::Ptr<NiNode>> nodes{};
};
template<class NiNode>
std::istream &operator>>(std::istream &is, NodeSet<NiNode> &t) {
  io::readBytes(is, t.numNodes);
  t.nodes.reserve(t.numNodes);
  for (basic::UInt i = 0; i < t.numNodes; ++i) {
    basic::Ptr<NiNode> node{};
    io::readBytes(node.val);
    t.nodes.push_back(node);
  }
  return is;
}

// ver > 10.1.0.0
struct ShortString {
  basic::Byte length{};
  // Null-terminated string, length includes null-terminator
  std::vector<basic::Char> value{};
};
std::istream &operator>>(std::istream &is, ShortString &t) {
  io::readBytes(is, t.length);
  auto len = static_cast<std::uint8_t>(t.length);
  t.value.reserve(len);
  for (typeof(len) i = 0; i < len; ++i) {
    basic::Char c{};
    io::readBytes(is, c);
    t.value.push_back(c);
  }
  return is;
}

struct ExportInfo {
  ShortString author{};
  ShortString processScript{};
  ShortString exportScript{};
};
std::istream &operator>>(std::istream &is, ExportInfo &t) {
  is >> t.author;
  is >> t.processScript;
  is >> t.exportScript;
  return is;
}

struct Header : Versionable {
  // Should be 'NetImmerse File Format x.x.x.x' for ver < 10.0.1.2
  // Should be 'Gamebryo File Format x.x.x.x' for ver > 10.1.0.0
  basic::HeaderString headerString{};

  VersionOptional<std::array<basic::LineString, 3>, Unbounded, "3.1.0.0"_ver>
      copyright{version};

  VersionOptional<basic::FileVersion, "3.1.0.1"_ver, Unbounded> ver{version};

  VersionOptional<Enum::EndianType, "20.0.0.3"_ver, Unbounded>
      endianType{version, Enum::EndianType::ENDIAN_LITTLE};

  VersionOptional<basic::ULittle32, "10.0.1.8"_ver, Unbounded> userVer{version};

  // Number of file objects
  VersionOptional<basic::ULittle32, "3.1.0.1"_ver, Unbounded>
      numBlocks{version};

  struct BSStreamHeader {
    // ver == 10.0.1.2 ||
    //  (ver == 20.2.0.7 || ver = 20.0.0.5 ||
    //    (ver >= 10.1.0.0 && ver <= 20.0.0.4 && userver <= 11))
    // && userver >= 3
    basic::ULittle32 userVersion2{};
    // ver == 10.0.1.2 ||
    //  (ver == 20.2.0.7 || ver == 20.0.0.5 ||
    //    (ver >= 10.1.0.0 && ver <= 20.0.0.4 && userver <= 11)))
    // && userver >= 3
    ExportInfo exportInfo{};
    // userVersion2 == 130
    ShortString maxFilepath{};
  };
  BSStreamHeader bsStreamHeader{};

  VersionOptional<ByteArray, "30.0.0.0"_ver, Unbounded> metadata{version};

  // Number of object types
  VersionOptional<basic::UShort, "5.0.0.1"_ver, Unbounded>
      numBlockTypes{version};

  // List of object types
  VersionOptional<std::vector<SizedString>, "5.0.0.1"_ver, Unbounded>
      blockTypes{version};

  // Map of objects into object types; i-th entry is the index in blockTypes
  // corresponding to the type of the i-th object.
  VersionOptional<std::vector<basic::BlockTypeIndex>, "5.0.0.1"_ver, Unbounded>
      blockTypeIndices{version};

  VersionOptional<std::vector<basic::UInt>, "20.2.0.5"_ver, Unbounded>
      blockSizes{version};

  VersionOptional<basic::UInt, "20.1.0.1"_ver, Unbounded> numStrings{version};

  VersionOptional<basic::UInt, "20.1.0.1"_ver, Unbounded>
      maxStringLength{version};

  VersionOptional<std::vector<SizedString>, "20.1.0.1"_ver, Unbounded>
      strings{version};

  VersionOptional<basic::UInt, "5.0.0.6"_ver, Unbounded> numGroups{version};

  VersionOptional<std::vector<basic::UInt>, "5.0.0.6"_ver, Unbounded>
      groups{version};

  explicit Header(uint32_t version) : Versionable(version) {}
};
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
    t.blockTypeIndices->reserve(t.numBlocks.value());
    for (basic::ULittle32 i = 0; i < t.numBlocks.value(); ++i) {
      basic::BlockTypeIndex b{};
      io::readBytes(is, b);
      t.blockTypeIndices->push_back(b);
    }
  }

  if (t.blockSizes) {
    t.blockSizes->reserve(t.numBlocks.value());
    for (basic::ULittle32 i = 0; i < t.numBlocks.value(); ++i) {
      basic::UInt s{};
      io::readBytes(is, s);
      t.blockSizes->push_back(s);
    }
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
    t.groups->reserve(t.numGroups.value());
    for (basic::UInt i = 0; i < t.numGroups.value(); ++i) {
      basic::UInt g{};
      io::readBytes(is, g);
      t.groups->push_back(g);
    }
  }

  return is;
}

} // namespace compound
} // namespace nif

#endif // OPENOBLIVION_NIF_COMPOUND_HPP
