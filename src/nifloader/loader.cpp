#include "nifloader/loader.hpp"
#include "settings.hpp"
#include "nif/basic.hpp"
#include "nif/bhk.hpp"
#include "nif/compound.hpp"
#include "nif/niobject.hpp"
#include <boost/format.hpp>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <string>
#include <string_view>

namespace nifloader {

nif::Version peekVersion(std::istream &is) {
  const auto startPos{is.tellg()};
  nif::basic::HeaderString headerVersion{};
  is >> headerVersion;
  const std::string_view headerStr{headerVersion.str};

  // Sanity check; header should start with 'Gamebryo' or 'NetImmerse'
  const auto firstWordPos{headerStr.find_first_of(' ')};
  if (firstWordPos == std::string_view::npos || firstWordPos == 0) {
    is.seekg(0);
    throw std::runtime_error("Invalid nif header");
  }
  const auto formatName{headerStr.substr(0, firstWordPos)};
  if (formatName != "Gamebryo" && formatName != "NetImmerse") {
    is.seekg(0);
    throw std::runtime_error(boost::str(boost::format(
        "Invalid nif header. Expected 'NetImmerse' or 'Gamebryo', found %s")
                                            % formatName));
  }

  // Now proceed with finding the version
  const auto lastWordPos{headerStr.find_last_of(' ')};
  const auto version{headerStr.substr(lastWordPos + 1, std::string_view::npos)};
  is.seekg(startPos);
  return nif::verOf(version);
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
  auto logger{spdlog::get(settings::log)};

  const Version nifVersion{peekVersion(is)};
  compound::Header header{nifVersion};
  is >> header;

  if (!header.numBlocks || *header.numBlocks == 0) {
    // File is empty, we can stop
    return BlockGraph{};
  }
  const auto numBlocks{*header.numBlocks};

  // Nif file uses a list of unique block types and pointers from each block
  // into the list. Undo this and construct a list of block types.
  std::vector<std::string> blockTypes{};
  blockTypes.reserve(numBlocks);
  if (header.numBlockTypes && header.blockTypes && header.blockTypeIndices) {
    for (auto index : *header.blockTypeIndices) {
      const compound::SizedString &type{(*header.blockTypes)[index]};
      blockTypes.emplace_back(type.value.begin(), type.value.end());
    }
  } else {
    // The block types are written directly before their data.
    // TODO: This is not an error.
    throw std::runtime_error("nif file has no block types");
  }

  std::vector<uint32_t> groups{};
  if (header.groups) {
    groups = *header.groups;
  }

  const auto &blockAddVertexMap = getAddVertexMap();

  // Helper function used below, checks if a reference points to a valid block.
  // The block still might have an incompatible type though.
  auto isRefValid = [numBlocks](auto &&ref) {
    const auto refInt{static_cast<int32_t>(ref)};
    return refInt > 0 && refInt < numBlocks;
  };

  // The rest of the file is a series of NiObjects, called blocks, whose types
  // are given in the corresponding entries of blockTypes. Some of the blocks
  // have children, so the blocks form a forest (i.e. a set of trees)
  // The vertex index of each block in the tree will be the same as its index
  // in the nif file. There is an edge from block A to block B if B is a child
  // of A. Blocks may have pointers and references to other blocks, which can
  // create (weak) cycles.
  BlockGraph blocks{numBlocks};
  for (unsigned long i = 0; i < numBlocks; ++i) {
    const auto &blockType = blockTypes[i];
    auto vertexAdder = blockAddVertexMap.find(blockType);
    if (vertexAdder != blockAddVertexMap.end()) {
      const auto &func = vertexAdder->second;
      std::invoke(func, blocks, i, nifVersion, is);
      logger->trace("Read block {} ({})", i, blockType);
    } else if (blockType == "NiNode") {
      // An alternative to the make-do-move idiom (whatever it's called) used
      // with unique_ptr. This gains const, looks cleaner for polymorphic_value,
      // and centralises the blocks[i] construction. It swaps a move and default
      // construct (of blocks[i], implicit when calling the first addEdge) for a
      // copy, so is probably slower.
      const auto block = [&is, nifVersion]() {
        auto b{jbcoe::make_polymorphic_value<NiNode>(nifVersion)};
        b->read(is);
        return b;
      }();
      blocks[i] = jbcoe::polymorphic_value<NiObject>(block);

      // Make an edge to each child
      for (auto child : block->children) {
        if (isRefValid(child)) addEdge(blocks, i, child);
      }

      // Make an edge to each NiExtraData
      // TODO: Support extra data linked list
      if (block->extraDataArray) {
        for (auto xtra : *(block->extraDataArray)) {
          if (isRefValid(xtra)) addEdge(blocks, i, xtra);
        }
      }

      // Make an edge to the controller
      if (block->controller) {
        const auto cont{*(block->controller)};
        if (isRefValid(cont)) addEdge(blocks, i, cont);
      }

      // Make an edge to each NiProperty
      for (auto prop : block->properties) {
        if (isRefValid(prop)) addEdge(blocks, i, prop);
      }

      // Make an edge to the collision object
      if (block->collisionObject) {
        const auto col{*(block->collisionObject)};
        if (isRefValid(col)) addEdge(blocks, i, col);
      }

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

} // namespace nifloader

