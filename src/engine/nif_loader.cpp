#include "engine/nif_loader.hpp"
#include "engine/ogre_data_stream_wrapper.hpp"
#include "io/memstream.hpp"
#include "nif/basic.hpp"
#include "nif/compound.hpp"
#include "nif/niobject.hpp"

#include <OgreMesh.h>
#include <OgreResourceGroupManager.h>
#include <streambuf>

namespace engine {

nif::Version NifLoader::peekVersion(std::istream &is) {
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
    throw std::runtime_error(std::string(
        "Invalid nif header. Expected 'NetImmerse' or 'Gamebryo', found ")
                                 .append(formatName));
  }

  // Now proceed with finding the version
  auto lastWordPos = headerVersion.str.find_last_of(' ');
  auto versionString =
      headerVersion.str.substr(lastWordPos + 1, std::string::npos);
  is.seekg(0);
  return nif::verOf(versionString.c_str(), versionString.length());
}

void NifLoader::loadResource(Ogre::Resource *resource) {
  auto mesh = dynamic_cast<Ogre::Mesh *>(resource);
  // TODO: Handle this properly
  assert(mesh != nullptr);

  // TODO: If the mesh doesn't exist, dynamically generate a placeholder
  auto ogreDataStream = Ogre::ResourceGroupManager::getSingletonPtr()
      ->openResource(mesh->getName(), mesh->getGroup());

  auto ogreDataStreamBuffer = OgreDataStreamWrapper{ogreDataStream};
  std::istream is{&ogreDataStreamBuffer};

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
    return;
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

  // The rest of the file is a series of NiObjects, called blocks, whose types
  // are given in the corresponding entries of blockTypes. Some of the blocks
  // have children, so the blocks form a forest (i.e. a set of trees).
  // The vertex index of each block in the tree will be the same as its index
  // in the nif file. There is an edge from block A to block B if B is a child
  // of A.
  BlockGraph blocks{numBlocks};
  for (auto i = 0; i < numBlocks; ++i) {
    auto &blockType = *blockTypes[i];
    if (blockType == "NiNode") {
      auto niNode = std::make_shared<NiNode>(nifVersion);
      niNode->read(is);
      for (const auto &child : niNode->children) {
        // TODO: Check that the child is a valid reference.
        auto parent = static_cast<BlockGraph::vertex_descriptor>(i);
        addEdge(blocks, parent, child);
      }
      blocks[i] = std::move(niNode);
    } else if (blockType == "NiTriShape") {
      auto niTriShape = std::make_shared<NiTriShape>(nifVersion);
      niTriShape->read(is);
      blocks[i] = std::move(niTriShape);
    } else {
      // TODO: Implement the other blocks
      break;
    }
  }
}

} // namespace engine

