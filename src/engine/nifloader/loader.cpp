#include "engine/conversions.hpp"
#include "engine/nifloader/loader.hpp"
#include "engine/nifloader/loader_state.hpp"
#include "ogre/ogre_stream_wrappers.hpp"
#include "engine/settings.hpp"
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
#include <spdlog/spdlog.h>
#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <nif/bhk.hpp>

namespace engine::nifloader {

nif::Version peekVersion(std::istream &is) {
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

const AddVertexMap &getAddVertexMap() {
  const static AddVertexMap map{
      {"NiFloatData", &addVertex<nif::NiFloatData>},
      {"NiKeyframeData", &addVertex<nif::NiKeyframeData>},
      {"NiTransformData", &addVertex<nif::NiTransformData>},
      {"NiPosData", &addVertex<nif::NiPosData>},
      {"NiStringPalette", &addVertex<nif::NiStringPalette>},
      {"NiExtraData", &addVertex<nif::NiExtraData>},
      {"NiBinaryExtraData", &addVertex<nif::NiBinaryExtraData>},
      {"NiIntegerExtraData", &addVertex<nif::NiIntegerExtraData>},
      {"NiStringExtraData", &addVertex<nif::NiStringExtraData>},
      {"NiTextKeyExtraData", &addVertex<nif::NiTextKeyExtraData>},
      {"NiFloatInterpolator", &addVertex<nif::NiFloatInterpolator>},
      {"NiTransformInterpolator", &addVertex<nif::NiTransformInterpolator>},
      {"NiPoint3Interpolator", &addVertex<nif::NiPoint3Interpolator>},
      {"NiBlendPoint3Interpolator", &addVertex<nif::NiBlendPoint3Interpolator>},
      {"BSXFlags", &addVertex<nif::BSXFlags>},
      {"NiSequence", &addVertex<nif::NiSequence>},
      {"NiControllerSequence", &addVertex<nif::NiControllerSequence>},
      {"NiDefaultAVObjectPalette", &addVertex<nif::NiDefaultAVObjectPalette>},
      {"NiMultiTargetTransformController",
       &addVertex<nif::NiMultiTargetTransformController>},
      {"NiMaterialColorController", &addVertex<nif::NiMaterialColorController>},
      {"NiControllerManager", &addVertex<nif::NiControllerManager>},
      {"NiMaterialProperty", &addVertex<nif::NiMaterialProperty>},
      {"NiTexturingProperty", &addVertex<nif::NiTexturingProperty>},
      {"NiStencilProperty", &addVertex<nif::NiStencilProperty>},
      {"NiVertexColorProperty", &addVertex<nif::NiVertexColorProperty>},
      {"NiAlphaProperty", &addVertex<nif::NiAlphaProperty>},
      {"NiSpecularProperty", &addVertex<nif::NiSpecularProperty>},
      {"NiCollisionObject", &addVertex<nif::NiCollisionObject>},
      /*{"NiNode", &addVertex<nif::NiNode>},*/
      {"NiAdditionalGeometryData", &addVertex<nif::NiAdditionalGeometryData>},
      {"NiGeometryData", &addVertex<nif::NiGeometryData>},
      {"NiTriShapeData", &addVertex<nif::NiTriShapeData>},
      {"NiTriStripsData", &addVertex<nif::NiTriStripsData>},
      {"NiSkinPartition", &addVertex<nif::NiSkinPartition>},
      {"NiSkinData", &addVertex<nif::NiSkinData>},
      {"NiSkinInstance", &addVertex<nif::NiSkinInstance>},
      {"NiGeometry", &addVertex<nif::NiGeometry>},
      {"NiTriShape", &addVertex<nif::NiTriShape>},
      {"NiTriStrips", &addVertex<nif::NiTriStrips>},
      {"NiSourceTexture", &addVertex<nif::NiSourceTexture>},
      {"bhkTransformShape", &addVertex<nif::bhk::TransformShape>},
      {"bhkSphereShape", &addVertex<nif::bhk::SphereShape>},
      {"bhkCapsuleShape", &addVertex<nif::bhk::CapsuleShape>},
      {"bhkBoxShape", &addVertex<nif::bhk::BoxShape>},
      {"bhkConvexVerticesShape", &addVertex<nif::bhk::ConvexVerticesShape>},
      {"bhkConvexTransformShape", &addVertex<nif::bhk::ConvexTransformShape>},
      {"bhkConvexSweepShape", &addVertex<nif::bhk::ConvexSweepShape>},
      {"bhkMoppBvTreeShape", &addVertex<nif::bhk::MoppBvTreeShape>},
      {"bhkListShape", &addVertex<nif::bhk::ListShape>},
      {"bhkPackedNiTriStripsShape",
       &addVertex<nif::bhk::PackedNiTriStripsShape>},
      {"bhkSimpleShapePhantom", &addVertex<nif::bhk::SimpleShapePhantom>},
      {"bhkLimitedHingeConstraint",
       &addVertex<nif::bhk::LimitedHingeConstraint>},
      {"bhkRagdollConstraint", &addVertex<nif::bhk::RagdollConstraint>},
      {"bhkRigidBody", &addVertex<nif::bhk::RigidBody>},
      {"bhkRigidBodyT", &addVertex<nif::bhk::RigidBodyT>},
      {"bhkCollisionObject", &addVertex<nif::bhk::CollisionObject>},
      {"hkPackedNiTriStripsData", &addVertex<nif::hk::PackedNiTriStripsData>},
  };
  return map;
}

BlockGraph createBlockGraph(std::istream &is) {
  using namespace nif;
  auto logger = spdlog::get(settings::log);

  Version nifVersion = peekVersion(is);
  compound::Header header{nifVersion};
  is >> header;

  if (!header.numBlocks || *header.numBlocks == 0) {
    // File is empty, we can stop
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

  const auto &blockAddVertexMap = getAddVertexMap();

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
      std::invoke(func, blocks, i, nifVersion, is);
      logger->trace("Read block {} ({})", i, blockType);
    } else if (blockType == "NiNode") {
      auto block = std::make_shared<NiNode>(nifVersion);
      block->read(is);

      // Make an edge to each child
      for (const auto &child : block->children) {
        auto childInt = static_cast<int32_t>(child);
        if (childInt > 0 && childInt < numBlocks) {
          addEdge(blocks, i, child);
        }
      }

      // Make an edge to each NiExtraData
      // TODO: Support extra data linked list
      if (block->extraDataArray) {
        for (const auto &xtra : *(block->extraDataArray)) {
          auto xtraInt = static_cast<int32_t>(xtra);
          if (xtraInt > 0 && xtraInt < numBlocks) {
            addEdge(blocks, i, xtra);
          }
        }
      }

      // Make an edge to the controller
      if (block->controller) {
        auto cont = *(block->controller);
        auto contInt = static_cast<int32_t>(cont);
        if (contInt > 0 && contInt < numBlocks) {
          addEdge(blocks, i, cont);
        }
      }

      // Make an edge to each NiProperty
      for (const auto &prop : block->properties) {
        auto propInt = static_cast<int32_t>(prop);
        if (propInt > 0 && propInt < numBlocks) {
          addEdge(blocks, i, prop);
        }
      }

      // Make an edge to the collision object
      if (block->collisionObject) {
        auto col = *(block->collisionObject);
        auto colInt = static_cast<int32_t>(col);
        if (colInt > 0 && colInt < numBlocks) {
          addEdge(blocks, i, col);
        }
      }

      blocks[i] = std::move(block);
      logger->trace("Read block {} (NiNode)", i);
    } else {
      // TODO: Implement the other blocks
      throw std::runtime_error(boost::str(
          boost::format("Found unsupported block type %s, cannot continue")
              % blockType));
    }
  }

  return blocks;
}

} // namespace engine::nifloader

