#include "engine/conversions.hpp"
#include "engine/nif_loader.hpp"
#include "engine/ogre_data_stream_wrapper.hpp"
#include "io/memstream.hpp"
#include "nif/basic.hpp"
#include "nif/compound.hpp"
#include "nif/niobject.hpp"

#include <OgreHardwareBufferManager.h>
#include <OgreLogManager.h>
#include <OgreMesh.h>
#include <OgreResourceGroupManager.h>
#include <OgreSubMesh.h>
#include <map>

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
    throw std::runtime_error(std::string(
        "Invalid nif header. Expected 'NetImmerse' or 'Gamebryo', found ")
                                 .append(formatName));
  }

  // Now proceed with finding the version
  auto lastWordPos = headerVersion.str.find_last_of(' ');
  auto versionString =
      headerVersion.str.substr(lastWordPos + 1, std::string::npos);
  is.seekg(startPos);
  return nif::verOf(versionString.c_str(), versionString.length());
}

void NifLoader::loadResource(Ogre::Resource *resource) {
  auto &logger = Ogre::LogManager::getSingleton();
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

  // Forgive me for this. Instead of doing an if-else over every block type,
  // calling addVertex() with identical arguments and duplication the if
  // condition in the template argument, we just query a map of member function
  // pointers. The macro helps writing the block type twice.
  // we use a map from
  typedef void(NifLoader::*addVertex_t)(BlockGraph &,
                                        BlockGraph::vertex_descriptor,
                                        nif::Version,
                                        std::istream &);
  std::map<std::string, addVertex_t> blockAddVertexMap{};

#define ENGINE_NIF_LOADER_ADD_BLOCK(blockType) \
    blockAddVertexMap.emplace(#blockType, &NifLoader::addVertex<blockType>);

  ENGINE_NIF_LOADER_ADD_BLOCK(NiExtraData);
  ENGINE_NIF_LOADER_ADD_BLOCK(NiBinaryExtraData);
  ENGINE_NIF_LOADER_ADD_BLOCK(NiIntegerExtraData);
  ENGINE_NIF_LOADER_ADD_BLOCK(NiStringExtraData);
  ENGINE_NIF_LOADER_ADD_BLOCK(BSXFlags);
  ENGINE_NIF_LOADER_ADD_BLOCK(NiMaterialProperty);
  ENGINE_NIF_LOADER_ADD_BLOCK(NiTexturingProperty);
  ENGINE_NIF_LOADER_ADD_BLOCK(NiCollisionObject);
  //ENGINE_NIF_LOADER_ADD_BLOCK(NiNode);
  ENGINE_NIF_LOADER_ADD_BLOCK(NiAdditionalGeometryData);
  ENGINE_NIF_LOADER_ADD_BLOCK(NiGeometryData);
  ENGINE_NIF_LOADER_ADD_BLOCK(NiTriShapeData);
  ENGINE_NIF_LOADER_ADD_BLOCK(NiSkinPartition);
  ENGINE_NIF_LOADER_ADD_BLOCK(NiSkinData);
  ENGINE_NIF_LOADER_ADD_BLOCK(NiSkinInstance);
  ENGINE_NIF_LOADER_ADD_BLOCK(NiGeometry);
  ENGINE_NIF_LOADER_ADD_BLOCK(NiTriShape);
  ENGINE_NIF_LOADER_ADD_BLOCK(NiSourceTexture);

  // The rest of the file is a series of NiObjects, called blocks, whose types
  // are given in the corresponding entries of blockTypes. Some of the blocks
  // have children, so the blocks form a forest (i.e. a set of trees).
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
      logger.logMessage(std::string("Read block ")
                            .append(std::to_string(i))
                            .append(" (")
                            .append(blockType)
                            .append(")"));
    } else if (blockType == "NiNode") {
      auto block = std::make_shared<NiNode>(nifVersion);
      block->read(is);
      for (const auto &child : block->children) {
        // TODO: Check that the child is a valid reference.
        addEdge(blocks, i, child);
      }
      blocks[i] = std::move(block);
    } else {
      // TODO: Implement the other blocks
      break;
    }
  }

  // The second pass is over the block graph
  for (const auto &index : blocks.vertex_set()) {
    auto niObject = blocks[index];
    auto &blockType = *blockTypes[index];
    if (blockType == "NiTriShape") {
      auto block = dynamic_cast<NiTriShape *>(niObject.get());

      // NiTriShape blocks determine discrete pieces of geometry with a single
      // material and texture, and so translate to Ogre::SubMesh objects.
      auto submesh = mesh->createSubMesh(block->name.str());
      submesh->useSharedVertices = false;

      // Ogre::SubMeshes cannot have transformations applied to them (that is
      // reserved for Ogre::SceneNodes), so we will apply it to all the vertex
      // information manually.
      Ogre::Vector3 translation = conversions::fromNif(block->translation);
      Ogre::Matrix3 rotation = conversions::fromNif(block->rotation);
      float scale = block->scale;

      // Follow the reference to the actual geometry data
      auto dataRef = static_cast<int32_t>(block->data);
      auto data = dynamic_cast<NiTriShapeData *>(blocks[dataRef].get());

      // It seems that Ogre expects this to be heap allocated, and will
      // deallocate it when the mesh is unloaded. Still, it makes me uneasy.
      submesh->vertexData = new Ogre::VertexData();
      submesh->vertexData->vertexCount = data->numVertices;
      auto vertDecl = submesh->vertexData->vertexDeclaration;
      auto vertBind = submesh->vertexData->vertexBufferBinding;
      auto hwBufPtr = Ogre::HardwareBufferManager::getSingletonPtr();

      // Specify the order of data in the vertex buffer. This is per vertex,
      // so the vertices, normals etc will have to be interleaved in the buffer.
      std::size_t offset{0};
      const unsigned short source{0};
      if (data->hasVertices) {
        vertDecl->addElement(source, offset, Ogre::VET_FLOAT3,
                             Ogre::VES_POSITION);
        offset += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
      }
      // TODO: Blend weights
      if (data->hasNormals) {
        vertDecl->addElement(source, offset, Ogre::VET_FLOAT3,
                             Ogre::VES_NORMAL);
        offset += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
      }
      // TODO: Diffuse color
      // TODO: Specular color
      if (!data->uvSets.empty()) {
        vertDecl->addElement(source, offset, Ogre::VET_FLOAT2,
                             Ogre::VES_TEXTURE_COORDINATES);
        offset += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT2);
      }

      // The final vertex buffer is the transpose of the block matrix
      // v1  v3  v3 ...
      // n1  n2  n3 ...
      // uv1 uv2 uv3 ...
      // ...
      // where v1 is a column vector of the first vertex position components,
      // and so on with a row for each of the elements of the vertex
      // declaration.
      // TODO: Is there an efficient transpose algorithm to make this abstraction worthwhile?
      std::vector<float> vertexBuffer(offset * data->numVertices);
      std::size_t localOffset{0};
      if (data->hasVertices) {
        auto it = vertexBuffer.begin();
        for (const auto &vertex : data->vertices) {
          *it = vertex.x;
          *(it + 1) = vertex.y;
          *(it + 2) = vertex.z;
          it += offset;
        }
        localOffset += 3;
      }
      // TODO: Blend weights
      if (data->hasNormals) {
        auto it = vertexBuffer.begin() + localOffset;
        for (const auto &normal : data->normals) {
          *it = normal.x;
          *(it + 1) = normal.y;
          *(it + 2) = normal.z;
          it += offset;
        }
        localOffset += 3;
      }
      // TODO: Diffuse color
      // TODO: Specular color
      if (!data->uvSets.empty()) {
        auto it = vertexBuffer.begin() + localOffset;
        // TODO: Support more than one UV set?
        for (const auto &uv : data->uvSets[0]) {
          *it = uv.u;
          *(it + 1) = uv.v;
          it += offset;
        }
        localOffset += 2;
      }

      // We can assume that compound::Triangle has no padding and std::vector
      // is sequential, so can avoid copying the faces.
      auto indexBuffer = reinterpret_cast<uint16_t *>(data->triangles.data());

      // Copy the vertex buffer into a hardware buffer, and link the buffer to
      // the vertex declaration.
      auto usage = Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY;
      auto hwVertexBuffer = hwBufPtr->createVertexBuffer(offset,
                                                         data->numVertices,
                                                         usage);
      hwVertexBuffer->writeData(0,
                                hwVertexBuffer->getSizeInBytes(),
                                vertexBuffer.data(),
                                true);
      vertBind->setBinding(source, hwVertexBuffer);

      // Copy the triangle (index) buffer into a hardware buffer.
      auto itype = Ogre::HardwareIndexBuffer::IT_16BIT;
      std::size_t numIndices = 3u * data->numTriangles;
      auto hwIndexBuffer = hwBufPtr->createIndexBuffer(itype,
                                                       numIndices,
                                                       usage);
      hwIndexBuffer->writeData(0,
                               hwIndexBuffer->getSizeInBytes(),
                               indexBuffer,
                               true);

      // Give the submesh the index data
      submesh->indexData->indexBuffer = hwIndexBuffer;
      submesh->indexData->indexCount = numIndices;
      submesh->indexData->indexStart = 0;
    }
  }
}

} // namespace engine

