#include "math/conversions.hpp"
#include "nif/basic.hpp"
#include "nif/compound.hpp"
#include "nif/niobject.hpp"
#include "nifloader/loader.hpp"
#include "nifloader/logging.hpp"
#include <algorithm>
#include <istream>
#include <string>
#include <string_view>

namespace oo {

nif::Version peekVersion(std::istream &is) {
  const auto startPos{is.tellg()};
  nif::basic::HeaderString headerVersion{};
  is >> headerVersion;
  const std::string_view headerStr{headerVersion.str};

  // Sanity check; header should start with 'Gamebryo' or 'NetImmerse'
  const auto firstWordPos{headerStr.find_first_of(' ')};
  if (firstWordPos == std::string_view::npos || firstWordPos == 0) {
    is.seekg(0);
    throw std::runtime_error("Invalid NIF header");
  }
  const auto formatName{headerStr.substr(0, firstWordPos)};
  if (formatName != "Gamebryo" && formatName != "NetImmerse") {
    is.seekg(0);
    oo::nifloaderLogger()->error("Invalid NIF header. Expected 'NetImmerse' or "
                                 "'Gamebryo', found '{}'.", formatName);
    throw std::runtime_error("Invalid NIF header");
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
      {"BSBound", &addVertex<nif::BSBound>},
      {"NiFloatInterpolator", &addVertex<nif::NiFloatInterpolator>},
      {"NiTransformInterpolator", &addVertex<nif::NiTransformInterpolator>},
      {"NiPoint3Interpolator", &addVertex<nif::NiPoint3Interpolator>},
      {"NiBlendPoint3Interpolator", &addVertex<nif::NiBlendPoint3Interpolator>},
      {"NiKeyframeController", &addVertex<nif::NiKeyframeController>},
      {"NiTransformController", &addVertex<nif::NiTransformController>},
      {"NiBoneLODController", &addVertex<nif::NiBoneLODController>},
      {"NiBSBoneLODController", &addVertex<nif::NiBSBoneLODController>},
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
      {"bhkBallAndSocketConstraint",
       &addVertex<nif::bhk::BallAndSocketConstraint>},
      {"bhkBreakableConstraint", &addVertex<nif::bhk::BreakableConstraint>},
      {"bhkHingeConstraint", &addVertex<nif::bhk::HingeConstraint>},
      {"bhkLimitedHingeConstraint",
       &addVertex<nif::bhk::LimitedHingeConstraint>},
      {"bhkMalleableConstraint", &addVertex<nif::bhk::MalleableConstraint>},
      {"bhkPrismaticConstraint", &addVertex<nif::bhk::PrismaticConstraint>},
      {"bhkRagdollConstraint", &addVertex<nif::bhk::RagdollConstraint>},
      {"bhkStiffSpringConstraint", &addVertex<nif::bhk::StiffSpringConstraint>},
      {"bhkRigidBody", &addVertex<nif::bhk::RigidBody>},
      {"bhkRigidBodyT", &addVertex<nif::bhk::RigidBodyT>},
      {"bhkCollisionObject", &addVertex<nif::bhk::CollisionObject>},
      {"bhkBlendController", &addVertex<nif::bhk::BlendController>},
      {"bhkBlendCollisionObject", &addVertex<nif::bhk::BlendCollisionObject>},
      {"hkPackedNiTriStripsData", &addVertex<nif::hk::PackedNiTriStripsData>},
  };
  return map;
}

BlockGraph createBlockGraph(std::istream &is) {
  const nif::Version nifVersion{oo::peekVersion(is)};
  nif::compound::Header header{nifVersion};
  is >> header;

  // If the file is empty, we can stop
  if (!header.numBlocks || *header.numBlocks == 0) return BlockGraph{};
  const auto numBlocks{*header.numBlocks};

  // Nif file uses a list of unique block types and pointers from each block
  // into the list. Undo this and construct a list of block types.
  std::vector<std::string> blockTypes{};
  blockTypes.reserve(numBlocks);
  if (header.numBlockTypes && header.blockTypes && header.blockTypeIndices) {
    for (auto index : *header.blockTypeIndices) {
      const nif::compound::SizedString &type{(*header.blockTypes)[index]};
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

  const auto &blockAddVertexMap{oo::getAddVertexMap()};

  // Helper function used below, checks if a reference points to a valid block.
  // The block still might have an incompatible type though.
  auto isRefValid = [numBlocks](auto &&ref) {
    const auto refInt{static_cast<int32_t>(ref)};
    return refInt > 0 && static_cast<std::size_t>(refInt) < numBlocks;
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
    const auto &blockType{blockTypes[i]};
    const auto vertexAdder{blockAddVertexMap.find(blockType)};
    if (vertexAdder != blockAddVertexMap.end()) {
      const auto &func{vertexAdder->second};
      std::invoke(func, blocks, i, nifVersion, is);
      oo::nifloaderLogger()->trace("Read block {} ({})", i, blockType);
    } else if (blockType == "NiNode") {
      // An alternative to the make-do-move idiom (whatever it's called) used
      // with unique_ptr. This gains const, looks cleaner for polymorphic_value,
      // and centralises the blocks[i] construction. It swaps a move and default
      // construct (of blocks[i], implicit when calling the first addEdge) for a
      // copy, so is probably slower.
      const auto block = [&is, nifVersion]() {
        auto b{jbcoe::make_polymorphic_value<nif::NiNode>(nifVersion)};
        b->read(is);
        return b;
      }();
      blocks[i] = jbcoe::polymorphic_value<nif::NiObject>(block);

      // Make an edge to each NiExtraData
      // TODO: Support extra data linked list
      if (block->extraDataArray) {
        for (auto xtra : *(block->extraDataArray)) {
          if (isRefValid(xtra)) oo::addEdge(blocks, i, xtra);
        }
      }

      // Make an edge to the controller
      if (block->controller) {
        const auto cont{*(block->controller)};
        if (isRefValid(cont)) oo::addEdge(blocks, i, cont);
      }

      // Make an edge to each NiProperty
      for (auto prop : block->properties) {
        if (isRefValid(prop)) oo::addEdge(blocks, i, prop);
      }

      // Make an edge to the collision object
      if (block->collisionObject) {
        const auto col{*(block->collisionObject)};
        if (isRefValid(col)) oo::addEdge(blocks, i, col);
      }

      // Make an edge to each child
      for (auto child : block->children) {
        if (isRefValid(child)) oo::addEdge(blocks, i, child);
      }

      oo::nifloaderLogger()->trace("Read block {} (NiNode)", i);
    } else {
      // TODO: Implement the other blocks
      oo::nifloaderLogger()->error("Unsupported block {} ({})", i, blockType);
      throw std::runtime_error("Unsupported block type");
    }
  }

  return blocks;
}

Ogre::Matrix4 getTransform(const nif::NiAVObject &block) {
  const Ogre::Vector3 translation{oo::fromBSCoordinates(block.translation)};

  const Ogre::Quaternion rotation = [&block]() {
    const auto m{oo::fromBSCoordinates(block.rotation)};
    return Ogre::Quaternion{m};
  }();

  const Ogre::Vector3 scale{block.scale, block.scale, block.scale};

  Ogre::Matrix4 trans;
  trans.makeTransform(translation, scale, rotation);

  return trans;
}

} // namespace oo

