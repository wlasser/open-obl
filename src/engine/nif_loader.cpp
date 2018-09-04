#include "engine/conversions.hpp"
#include "engine/nif_loader.hpp"
#include "engine/ogre_stream_wrappers.hpp"
#include "io/memstream.hpp"
#include "nif/basic.hpp"
#include "nif/compound.hpp"
#include "nif/niobject.hpp"

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

NifLoader::BlockGraph NifLoader::createBlockGraph(std::istream &is,
                                                  Ogre::Mesh *mesh) {
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

  const std::map<std::string, NifLoader::addVertex_t> blockAddVertexMap{
      {"NiExtraData", &NifLoader::addVertex<nif::NiExtraData>},
      {"NiBinaryExtraData", &NifLoader::addVertex<nif::NiBinaryExtraData>},
      {"NiIntegerExtraData", &NifLoader::addVertex<nif::NiIntegerExtraData>},
      {"NiStringExtraData", &NifLoader::addVertex<nif::NiStringExtraData>},
      {"BSXFlags", &NifLoader::addVertex<nif::BSXFlags>},
      {"NiMaterialProperty", &NifLoader::addVertex<nif::NiMaterialProperty>},
      {"NiTexturingProperty", &NifLoader::addVertex<nif::NiTexturingProperty>},
      {"NiCollisionObject", &NifLoader::addVertex<nif::NiCollisionObject>},
      // NiNode
      {"NiAdditionalGeometryData",
       &NifLoader::addVertex<nif::NiAdditionalGeometryData>},
      {"NiGeometryData", &NifLoader::addVertex<nif::NiGeometryData>},
      {"NiTriShapeData", &NifLoader::addVertex<nif::NiTriShapeData>},
      {"NiSkinPartition", &NifLoader::addVertex<nif::NiSkinPartition>},
      {"NiSkinData", &NifLoader::addVertex<nif::NiSkinData>},
      {"NiSkinInstance", &NifLoader::addVertex<nif::NiSkinInstance>},
      {"NiGeometry", &NifLoader::addVertex<nif::NiGeometry>},
      {"NiTriShape", &NifLoader::addVertex<nif::NiTriShape>},
      {"NiSourceTexture", &NifLoader::addVertex<nif::NiSourceTexture>}
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
      logger.logMessage(std::string("Read block ")
                            .append(std::to_string(i))
                            .append(" (NiNode)"));
    } else {
      // TODO: Implement the other blocks
      break;
    }
  }

  return blocks;
}

Ogre::AxisAlignedBox NifLoader::parseNiTriShape(nif::NiTriShape *block,
                                                const BlockGraph &blocks,
                                                Ogre::Mesh *mesh) {
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

  // Ogre requires a bounding box for the mesh. Since we have to iterate over
  // the vertices anyway, we can compute it easily by tracking the min and max
  // of the x, y, z coordinates of all the vertices in all the blocks.
  Ogre::Vector3 boundingBoxMin{
      std::numeric_limits<float>::max(),
      std::numeric_limits<float>::max(),
      std::numeric_limits<float>::max()
  };
  Ogre::Vector3 boundingBoxMax{
      std::numeric_limits<float>::min(),
      std::numeric_limits<float>::min(),
      std::numeric_limits<float>::min()
  };

  // Follow the reference to the actual geometry data
  auto dataRef = static_cast<int32_t>(block->data);
  auto data = dynamic_cast<nif::NiTriShapeData *>(blocks[dataRef].get());

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
  std::size_t bytesPerVertex{0};
  const unsigned short source{0};
  if (data->hasVertices) {
    vertDecl->addElement(source, offset, Ogre::VET_FLOAT3,
                         Ogre::VES_POSITION);
    bytesPerVertex += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
    offset += 3;
  }
  // TODO: Blend weights
  if (data->hasNormals) {
    vertDecl->addElement(source, offset, Ogre::VET_FLOAT3,
                         Ogre::VES_NORMAL);
    bytesPerVertex += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
    offset += 3;
  }
  // TODO: Diffuse color
  // TODO: Specular color
  if (!data->uvSets.empty()) {
    vertDecl->addElement(source, offset, Ogre::VET_FLOAT2,
                         Ogre::VES_TEXTURE_COORDINATES);
    bytesPerVertex += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT2);
    offset += 2;
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
      auto v = scale * rotation * conversions::fromNif(vertex) + translation;
      *it = v.x;
      *(it + 1) = v.y;
      *(it + 2) = v.z;
      it += offset;
      // While we're iterating, check if we need to enlarge the bounding box
      if (v.x < boundingBoxMin.x) boundingBoxMin.x = v.x;
      else if (v.x > boundingBoxMax.x) boundingBoxMax.x = v.x;
      if (v.y < boundingBoxMin.y) boundingBoxMin.y = v.y;
      else if (v.y > boundingBoxMax.y) boundingBoxMax.y = v.y;
      if (v.z < boundingBoxMin.z) boundingBoxMin.z = v.z;
      else if (v.z > boundingBoxMax.z) boundingBoxMax.z = v.z;
    }
    localOffset += 3;
  }
  // TODO: Blend weights
  if (data->hasNormals) {
    auto it = vertexBuffer.begin() + localOffset;
    for (const auto &normal : data->normals) {
      auto n = rotation * conversions::fromNif(normal) + translation;
      *it = n.x;
      *(it + 1) = n.y;
      *(it + 2) = n.z;
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

  // Ogre expects an anticlockwise winding order
  if (data->hasNormals) {
    for (const auto &tri : data->triangles) {
      using conversions::fromNif;
      // Compute the triangle normal
      auto v1 = fromNif(data->vertices[tri.v1]);
      auto v2 = fromNif(data->vertices[tri.v2]);
      auto v3 = fromNif(data->vertices[tri.v3]);
      auto expected = (v2 - v1).crossProduct(v3 - v1);
      // Estimate the triangle normal from the vertex normals
      auto n1 = fromNif(data->normals[tri.v1]);
      auto n2 = fromNif(data->normals[tri.v2]);
      auto n3 = fromNif(data->normals[tri.v3]);
      auto actual = (n1 + n2 + n3) / 3.0f;
      // Coordinate system is right-handed so this is positive for an
      // anticlockwise winding order and negative for a clockwise order.
      if (expected.dotProduct(actual) < 0.0f) {
        logger.logMessage(Ogre::LogMessageLevel::LML_WARNING,
                          "Clockwise triangle winding order, expected anticlockwise");
      }
    }
  }

  // Copy the vertex buffer into a hardware buffer, and link the buffer to
  // the vertex declaration.
  auto usage = Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY;
  auto hwVertexBuffer = hwBufPtr->createVertexBuffer(bytesPerVertex,
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

  return Ogre::AxisAlignedBox(boundingBoxMin, boundingBoxMax);
}

void NifLoader::parseNiMaterialProperty(nif::NiMaterialProperty *block,
                                        const engine::NifLoader::BlockGraph &blocks,
                                        Ogre::Mesh *mesh) {
  auto &materialManager = Ogre::MaterialManager::getSingleton();
  auto material = materialManager.create(block->name.str(), mesh->getGroup());

  auto technique = material->getTechnique(0);
  auto pass = technique->getPass(0);

  pass->setAmbient(conversions::fromNif(block->ambientColor));

  auto diffuse = conversions::fromNif(block->diffuseColor);
  diffuse.a = block->alpha;
  pass->setDiffuse(diffuse);

  auto specular = conversions::fromNif(block->specularColor);
  specular.a = block->alpha;
  pass->setSpecular(specular);

  pass->setEmissive(conversions::fromNif(block->emissiveColor));

  // TODO: How to convert from nif glossiness to Ogre shininess?
  pass->setShininess(block->glossiness);
}

void NifLoader::loadResource(Ogre::Resource *resource) {
  auto &logger = Ogre::LogManager::getSingleton();
  auto mesh = dynamic_cast<Ogre::Mesh *>(resource);
  // TODO: Handle this properly
  assert(mesh != nullptr);

  // TODO: If the mesh doesn't exist, dynamically generate a placeholder
  auto ogreDataStream = Ogre::ResourceGroupManager::getSingletonPtr()
      ->openResource(mesh->getName(), mesh->getGroup());

  auto ogreDataStreamBuffer = OgreDataStreambuf{ogreDataStream};
  std::istream is{&ogreDataStreamBuffer};

  auto blocks = createBlockGraph(is, mesh);

  Ogre::Vector3 boundingBoxMin{
      std::numeric_limits<float>::max(),
      std::numeric_limits<float>::max(),
      std::numeric_limits<float>::max()
  };
  Ogre::Vector3 boundingBoxMax{
      std::numeric_limits<float>::min(),
      std::numeric_limits<float>::min(),
      std::numeric_limits<float>::min()
  };

  // The second pass is over the block graph
  for (const auto &index : blocks.vertex_set()) {
    auto niObject = blocks[index];
    if (auto block = dynamic_cast<nif::NiTriShape *>(niObject.get())) {
      auto bbox = parseNiTriShape(block, blocks, mesh);

      auto min = bbox.getMinimum();
      boundingBoxMin.x = std::min(boundingBoxMin.x, min.x);
      boundingBoxMin.y = std::min(boundingBoxMin.y, min.y);
      boundingBoxMin.z = std::min(boundingBoxMin.z, min.z);

      auto max = bbox.getMaximum();
      boundingBoxMax.x = std::max(boundingBoxMax.x, max.x);
      boundingBoxMax.y = std::max(boundingBoxMax.y, max.y);
      boundingBoxMax.z = std::max(boundingBoxMax.z, max.z);
    } else if (auto
        block = dynamic_cast<nif::NiMaterialProperty *>(niObject.get())) {
      parseNiMaterialProperty(block, blocks, mesh);
    }
  }

  mesh->_setBounds(Ogre::AxisAlignedBox(boundingBoxMin, boundingBoxMax));
}

} // namespace engine

