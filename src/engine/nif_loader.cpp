#include "engine/conversions.hpp"
#include "engine/nif_loader.hpp"
#include "engine/nif_loader_state.hpp"
#include "engine/ogre_stream_wrappers.hpp"
#include "io/memstream.hpp"
#include "nif/basic.hpp"
#include "nif/bhk.hpp"
#include "nif/compound.hpp"
#include "nif/niobject.hpp"

#include <boost/format.hpp>
#include <boost/graph/copy.hpp>
#include <OgreHardwareBufferManager.h>
#include <OgreLogManager.h>
#include <OgreMaterial.h>
#include <OgreMaterialManager.h>
#include <OgreMesh.h>
#include <OgrePass.h>
#include <OgreResourceGroupManager.h>
#include <OgreSubMesh.h>
#include <OgreTechnique.h>
#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <nif/bhk.hpp>

namespace engine {

nif::Version NifLoader::peekVersion(std::istream &is) {
  auto startPos = is.tellg();
  nif::basic::HeaderString headerVersion{};
  is >> headerVersion;

  // Sanity check; header should start with 'Gamebryo' or 'NetImmerse'
  auto firstWordPos = headerVersion.str.find_first_of(' ');
  if (firstWordPos == std::string::npos || firstWordPos == 0) {
    is.seekg(0);
    throw std::runtime_error("Invalid nif header");
  }
  auto formatName = headerVersion.str.substr(0, firstWordPos);
  if (formatName != "Gamebryo" && formatName != "NetImmerse") {
    is.seekg(0);
    throw std::runtime_error(boost::str(boost::format(
        "Invalid nif header. Expected 'NetImmerse' or 'Gamebryo', found %s")
                                            % formatName));
  }

  // Now proceed with finding the version
  auto lastWordPos = headerVersion.str.find_last_of(' ');
  auto versionString =
      headerVersion.str.substr(lastWordPos + 1, std::string::npos);
  is.seekg(startPos);
  return nif::verOf(versionString.c_str(), versionString.length());
}

NifLoader::BlockGraph NifLoader::createBlockGraph(std::istream &is) {
  using namespace nif;

  Version nifVersion = peekVersion(is);

  compound::Header header{nifVersion};
  is >> header;

  std::optional<Version> userVersion{};
  if (header.userVer) {
    userVersion = *header.userVer;
  }

  std::optional<Version> userVersion2{};
  if (header.bsStreamHeader.userVersion2 != 0) {
    userVersion2 = header.bsStreamHeader.userVersion2;
  }

  if (!header.numBlocks || *header.numBlocks == 0) {
    // File is empty, we can stop. Technically this is ok for sufficiently low
    // versions, but we do not support those.
    // TODO: Make sure resource is in a usable state
    return BlockGraph{};
  }
  auto numBlocks = *header.numBlocks;

  std::vector<std::string> possibleBlockTypes{};
  if (header.numBlockTypes && header.blockTypes) {
    possibleBlockTypes.reserve(*header.numBlockTypes);
    for (const compound::SizedString &blockType : *header.blockTypes) {
      possibleBlockTypes.emplace_back(blockType.value.begin(),
                                      blockType.value.end());
    }
  } else {
    // In this case, the block types are written directly before their data,
    // similar to an esp file.
    // TODO: This is not an error
    throw std::runtime_error("nif file has no block types");
  }

  std::vector<std::string *> blockTypes{};
  if (header.blockTypeIndices) {
    blockTypes.reserve(*header.numBlocks);
    for (auto i = 0; i < numBlocks; ++i) {
      std::size_t index = (*header.blockTypeIndices)[i];
      blockTypes.push_back(&possibleBlockTypes[index]);
    }
  }

  std::vector<uint32_t> groups{};
  if (header.groups) {
    groups = *header.groups;
  }

  const std::map<std::string, addVertex_t> blockAddVertexMap{
      {"NiFloatData", &NifLoader::addVertex<nif::NiFloatData>},
      {"NiKeyframeData", &NifLoader::addVertex<nif::NiKeyframeData>},
      {"NiTransformData", &NifLoader::addVertex<nif::NiTransformData>},
      {"NiPosData", &NifLoader::addVertex<nif::NiPosData>},
      {"NiStringPalette", &NifLoader::addVertex<nif::NiStringPalette>},
      {"NiExtraData", &NifLoader::addVertex<nif::NiExtraData>},
      {"NiBinaryExtraData", &NifLoader::addVertex<nif::NiBinaryExtraData>},
      {"NiIntegerExtraData", &NifLoader::addVertex<nif::NiIntegerExtraData>},
      {"NiStringExtraData", &NifLoader::addVertex<nif::NiStringExtraData>},
      {"NiTextKeyExtraData", &NifLoader::addVertex<nif::NiTextKeyExtraData>},
      {"NiFloatInterpolator", &NifLoader::addVertex<nif::NiFloatInterpolator>},
      {"NiTransformInterpolator",
       &NifLoader::addVertex<nif::NiTransformInterpolator>},
      {"NiPoint3Interpolator",
       &NifLoader::addVertex<nif::NiPoint3Interpolator>},
      {"BSXFlags", &NifLoader::addVertex<nif::BSXFlags>},
      {"NiSequence", &NifLoader::addVertex<nif::NiSequence>},
      {"NiControllerSequence",
       &NifLoader::addVertex<nif::NiControllerSequence>},
      {"NiDefaultAVObjectPalette",
       &NifLoader::addVertex<nif::NiDefaultAVObjectPalette>},
      {"NiControllerManager", &NifLoader::addVertex<nif::NiControllerManager>},
      {"NiMaterialProperty", &NifLoader::addVertex<nif::NiMaterialProperty>},
      {"NiTexturingProperty", &NifLoader::addVertex<nif::NiTexturingProperty>},
      {"NiStencilProperty", &NifLoader::addVertex<nif::NiStencilProperty>},
      {"NiVertexColorProperty",
       &NifLoader::addVertex<nif::NiVertexColorProperty>},
      {"NiAlphaProperty", &NifLoader::addVertex<nif::NiAlphaProperty>},
      {"NiSpecularProperty", &NifLoader::addVertex<nif::NiSpecularProperty>},
      {"NiCollisionObject", &NifLoader::addVertex<nif::NiCollisionObject>},
      // NiNode
      {"NiAdditionalGeometryData",
       &NifLoader::addVertex<nif::NiAdditionalGeometryData>},
      {"NiGeometryData", &NifLoader::addVertex<nif::NiGeometryData>},
      {"NiTriShapeData", &NifLoader::addVertex<nif::NiTriShapeData>},
      {"NiTriStripsData", &NifLoader::addVertex<nif::NiTriStripsData>},
      {"NiSkinPartition", &NifLoader::addVertex<nif::NiSkinPartition>},
      {"NiSkinData", &NifLoader::addVertex<nif::NiSkinData>},
      {"NiSkinInstance", &NifLoader::addVertex<nif::NiSkinInstance>},
      {"NiGeometry", &NifLoader::addVertex<nif::NiGeometry>},
      {"NiTriShape", &NifLoader::addVertex<nif::NiTriShape>},
      {"NiTriStrips", &NifLoader::addVertex<nif::NiTriStrips>},
      {"NiSourceTexture", &NifLoader::addVertex<nif::NiSourceTexture>},
      {"bhkTransformShape", &NifLoader::addVertex<nif::bhk::TransformShape>},
      {"bhkSphereShape", &NifLoader::addVertex<nif::bhk::SphereShape>},
      {"bhkCapsuleShape", &NifLoader::addVertex<nif::bhk::CapsuleShape>},
      {"bhkBoxShape", &NifLoader::addVertex<nif::bhk::BoxShape>},
      {"bhkConvexVerticesShape",
       &NifLoader::addVertex<nif::bhk::ConvexVerticesShape>},
      {"bhkConvexTransformShape",
       &NifLoader::addVertex<nif::bhk::ConvexTransformShape>},
      {"bhkConvexSweepShape",
       &NifLoader::addVertex<nif::bhk::ConvexSweepShape>},
      {"bhkMoppBvTreeShape", &NifLoader::addVertex<nif::bhk::MoppBvTreeShape>},
      {"bhkPackedNiTriStripsShape",
       &NifLoader::addVertex<nif::bhk::PackedNiTriStripsShape>},
      {"bhkSimpleShapePhantom",
       &NifLoader::addVertex<nif::bhk::SimpleShapePhantom>},
      {"bhkRigidBody", &NifLoader::addVertex<nif::bhk::RigidBody>},
      {"bhkRigidBodyT", &NifLoader::addVertex<nif::bhk::RigidBodyT>},
      {"bhkCollisionObject", &NifLoader::addVertex<nif::bhk::CollisionObject>},
      {"hkPackedNiTriStripsData",
       &NifLoader::addVertex<nif::hk::PackedNiTriStripsData>},
  };

  // The rest of the file is a series of NiObjects, called blocks, whose types
  // are given in the corresponding entries of blockTypes. Some of the blocks
  // have children, so the blocks form a forest (i.e. a set of trees)
  // The vertex index of each block in the tree will be the same as its index
  // in the nif file. There is an edge from block A to block B if B is a child
  // of A. Because the blocks can have references and pointers to other blocks,
  // it is simplest to perform two passes; the first pass reads the data, the
  // second constructs the mesh.
  BlockGraph blocks{numBlocks};
  for (unsigned long i = 0; i < numBlocks; ++i) {
    auto &blockType = *blockTypes[i];
    auto vertexAdder = blockAddVertexMap.find(blockType);
    if (vertexAdder != blockAddVertexMap.end()) {
      const auto &func = vertexAdder->second;
      (this->*func)(blocks, i, nifVersion, is);
      logger->logMessage(boost::str(
          boost::format("Read block %d (%i)") % i % blockType));
    } else if (blockType == "NiNode") {
      auto block = std::make_shared<NiNode>(nifVersion);
      block->read(is);
      for (const auto &child : block->children) {
        auto childInt = static_cast<int32_t>(child);
        if (childInt > 0 && childInt < numBlocks) {
          addEdge(blocks, i, child);
        }
      }
      blocks[i] = std::move(block);
      logger->logMessage(boost::str(
          boost::format("Read block %d (NiNode)") % i));
    } else {
      // TODO: Implement the other blocks
      throw std::runtime_error(boost::str(
          boost::format("Found unsupported block type %s, cannot continue")
              % blockType));
    }
  }

  return blocks;
}

void NifLoader::loadResource(Ogre::Resource *resource) {
  if (!logger) logger = Ogre::LogManager::getSingletonPtr();
  auto mesh = dynamic_cast<Ogre::Mesh *>(resource);
  // TODO: Handle this properly
  assert(mesh != nullptr);

  logger->logMessage(boost::str(
      boost::format("Loading %s") % resource->getName()));

  // TODO: If the mesh doesn't exist, dynamically generate a placeholder
  auto ogreDataStream = Ogre::ResourceGroupManager::getSingletonPtr()
      ->openResource(mesh->getName(), mesh->getGroup());

  auto ogreDataStreamBuffer = OgreDataStreambuf{ogreDataStream};
  std::istream is{&ogreDataStreamBuffer};

  auto blocks = createBlockGraph(is);

  NifLoaderState instance(mesh, blocks);
}

void NifLoader::dumpAsObj(std::istream &in, std::ostream &out) {
  std::vector<Ogre::Vector3> vertices;
  std::vector<Ogre::Vector3> normals;
  std::vector<Ogre::Vector2> uvs;
  std::vector<Ogre::Vector<3, std::size_t>> tris;
  std::size_t offset = 0;

  auto blocks = createBlockGraph(in);

  for (const auto &blockVertex : blocks.vertex_set()) {
    auto block = blocks[blockVertex].get();
    if (auto niTriShape = dynamic_cast<nif::NiTriShape *>(block)) {
      Ogre::Vector3 translation{conversions::fromNif(niTriShape->translation)};
      Ogre::Matrix3 rotationMatrix{conversions::fromNif(niTriShape->rotation)};
      Ogre::Quaternion rotation{rotationMatrix};
      Ogre::Vector3 scale{
          niTriShape->scale,
          niTriShape->scale,
          niTriShape->scale
      };
      Ogre::Vector3 invScale{
          1.0f / niTriShape->scale,
          1.0f / niTriShape->scale,
          1.0f / niTriShape->scale
      };
      Ogre::Matrix4 transformation{};
      transformation.makeTransform(translation, scale, rotation);
      Ogre::Matrix4 normalTransformation{};
      normalTransformation.makeTransform(Ogre::Vector3::ZERO,
                                         invScale,
                                         rotation);

      auto dataRef = static_cast<int32_t>(niTriShape->data);
      auto data = dynamic_cast<nif::NiTriShapeData *>(blocks[dataRef].get());

      for (const auto &vertex : data->vertices) {
        auto bsV = transformation * Ogre::Vector4(conversions::fromNif(vertex));
        auto v = conversions::fromBSCoordinates(bsV.xyz());
        vertices.push_back(v);
      }

      for (const auto &normal : data->normals) {
        auto bsN =
            normalTransformation * Ogre::Vector4(conversions::fromNif(normal));
        auto n = conversions::fromBSCoordinates(bsN.xyz());
        normals.push_back(n);
      }

      for (const auto &uv : data->uvSets[0]) {
        uvs.emplace_back(uv.u, uv.v);
      }

      for (const auto &tri : data->triangles) {
        tris.emplace_back(tri.v1 + offset + 1,
                          tri.v2 + offset + 1,
                          tri.v3 + offset + 1);
      }
      offset += data->numVertices;
    }
  }

  auto vFmt = boost::format("v %f %f %f\n");
  for (const auto &v : vertices) {
    out << (vFmt % v.x % v.y % v.z);
  }

  auto vnFmt = boost::format("vn %f %f %f\n");
  for (const auto &n : normals) {
    out << (vnFmt % n.x % n.y % n.z);
  }

  auto vtFmt = boost::format("vt %f %f\n");
  for (const auto &uv : uvs) {
    out << (vtFmt % uv.x % uv.y);
  }

  auto fFmt = boost::format("f %1%/%1%/%1% %2%/%2%/%2% %3%/%3%/%3%\n");
  for (const auto &tri : tris) {
    out << (fFmt % tri[0] % tri[1] % tri[2]);
  }
}
} // namespace engine

