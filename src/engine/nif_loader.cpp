#include "engine/conversions.hpp"
#include "engine/nif_loader.hpp"
#include "engine/ogre_stream_wrappers.hpp"
#include "io/memstream.hpp"
#include "nif/basic.hpp"
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
      {"NiExtraData", &NifLoader::addVertex<nif::NiExtraData>},
      {"NiBinaryExtraData", &NifLoader::addVertex<nif::NiBinaryExtraData>},
      {"NiIntegerExtraData",
       &NifLoader::addVertex<nif::NiIntegerExtraData>},
      {"NiStringExtraData", &NifLoader::addVertex<nif::NiStringExtraData>},
      {"BSXFlags", &NifLoader::addVertex<nif::BSXFlags>},
      {"NiMaterialProperty",
       &NifLoader::addVertex<nif::NiMaterialProperty>},
      {"NiTexturingProperty",
       &NifLoader::addVertex<nif::NiTexturingProperty>},
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

Ogre::AxisAlignedBox NifLoaderState::getBoundingBox(nif::NiTriShapeData *block,
                                                    Ogre::Matrix4 transformation) {
  const auto fltMin = std::numeric_limits<float>::min();
  const auto fltMax = std::numeric_limits<float>::max();
  Ogre::Vector3 bboxMin{fltMax, fltMax, fltMax};
  Ogre::Vector3 bboxMax{fltMin, fltMin, fltMin};

  if (!block->hasVertices || block->vertices.empty()) return {};

  for (const auto &vertex : block->vertices) {
    auto bsV = transformation * Ogre::Vector4(conversions::fromNif(vertex));
    auto v = conversions::fromBSCoordinates(bsV.xyz());
    if (v.x < bboxMin.x) bboxMin.x = v.x;
    else if (v.x > bboxMax.x) bboxMax.x = v.x;
    if (v.y < bboxMin.y) bboxMin.y = v.y;
    else if (v.y > bboxMax.y) bboxMax.y = v.y;
    if (v.z < bboxMin.z) bboxMin.z = v.z;
    else if (v.z > bboxMax.z) bboxMax.z = v.z;
  }
  return {bboxMin, bboxMax};
}

std::unique_ptr<Ogre::VertexData>
NifLoaderState::generateVertexData(nif::NiTriShapeData *block,
                                   Ogre::Matrix4 transformation) {
  // Ogre expects a heap allocated raw pointer, but to improve exception safety
  // we construct an unique_ptr then relinquish control of it to Ogre.
  auto vertexData = std::make_unique<Ogre::VertexData>();
  vertexData->vertexCount = block->numVertices;
  auto vertDecl = vertexData->vertexDeclaration;
  auto vertBind = vertexData->vertexBufferBinding;
  auto hwBufPtr = Ogre::HardwareBufferManager::getSingletonPtr();

  // Specify the order of data in the vertex buffer. This is per vertex,
  // so the vertices, normals etc will have to be interleaved in the buffer.
  std::size_t offset{0};
  std::size_t bytesPerVertex{0};
  const unsigned short source{0};
  if (block->hasVertices) {
    vertDecl->addElement(source, bytesPerVertex, Ogre::VET_FLOAT3,
                         Ogre::VES_POSITION);
    bytesPerVertex += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
    offset += 3;
  }
  // TODO: Blend weights
  if (block->hasNormals) {
    vertDecl->addElement(source, bytesPerVertex, Ogre::VET_FLOAT3,
                         Ogre::VES_NORMAL);
    bytesPerVertex += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
    offset += 3;
  }
  // TODO: Diffuse color
  // TODO: Specular color
  if (!block->uvSets.empty()) {
    vertDecl->addElement(source, bytesPerVertex, Ogre::VET_FLOAT2,
                         Ogre::VES_TEXTURE_COORDINATES, 0);
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
  std::vector<float> vertexBuffer(offset * block->numVertices);
  std::size_t localOffset{0};
  if (block->hasVertices) {
    auto it = vertexBuffer.begin();
    for (const auto &vertex : block->vertices) {
      auto bsV = transformation * Ogre::Vector4(conversions::fromNif(vertex));
      auto v = conversions::fromBSCoordinates(bsV.xyz());
      *it = v.x;
      *(it + 1) = v.y;
      *(it + 2) = v.z;
      it += offset;
    }
    localOffset += 3;
  }
  // TODO: Blend weights
  if (block->hasNormals) {
    // Normal vectors are not translated and transform with the inverse
    // transpose of the transformation matrix
    auto normalTransformation = transformation;
    normalTransformation.setTrans(Ogre::Vector3::ZERO);
    normalTransformation = normalTransformation.inverse().transpose();
    auto it = vertexBuffer.begin() + localOffset;
    for (const auto &normal : block->normals) {
      auto bsN =
          normalTransformation * Ogre::Vector4(conversions::fromNif(normal));
      auto n = conversions::fromBSCoordinates(bsN.xyz());
      *it = n.x;
      *(it + 1) = n.y;
      *(it + 2) = n.z;
      it += offset;
    }
    localOffset += 3;
  }
  // TODO: Diffuse color
  // TODO: Specular color
  if (!block->uvSets.empty()) {
    auto it = vertexBuffer.begin() + localOffset;
    // TODO: Support more than one UV set?
    for (const auto &uv : block->uvSets[0]) {
      *it = uv.u;
      *(it + 1) = uv.v;
      it += offset;
    }
    localOffset += 2;
  }

  // Ogre expects an anticlockwise winding order
  if (block->hasNormals) {
    for (const auto &tri : block->triangles) {
      using conversions::fromNif;
      // Compute the triangle normal
      auto v1 = fromNif(block->vertices[tri.v1]);
      auto v2 = fromNif(block->vertices[tri.v2]);
      auto v3 = fromNif(block->vertices[tri.v3]);
      auto expected = (v2 - v1).crossProduct(v3 - v1);
      // Estimate the triangle normal from the vertex normals
      auto n1 = fromNif(block->normals[tri.v1]);
      auto n2 = fromNif(block->normals[tri.v2]);
      auto n3 = fromNif(block->normals[tri.v3]);
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
                                                     block->numVertices,
                                                     usage);
  hwVertexBuffer->writeData(0,
                            hwVertexBuffer->getSizeInBytes(),
                            vertexBuffer.data(),
                            true);
  vertBind->setBinding(source, hwVertexBuffer);

  return vertexData;
}

std::unique_ptr<Ogre::IndexData>
NifLoaderState::generateIndexData(nif::NiTriShapeData *block) {
  auto hwBufPtr = Ogre::HardwareBufferManager::getSingletonPtr();

  // We can assume that compound::Triangle has no padding and std::vector
  // is sequential, so can avoid copying the faces.
  auto indexBuffer = reinterpret_cast<uint16_t *>(block->triangles.data());

  auto usage = Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY;
  auto itype = Ogre::HardwareIndexBuffer::IT_16BIT;
  std::size_t numIndices = 3u * block->numTriangles;

  // Copy the triangle (index) buffer into a hardware buffer.
  auto hwIndexBuffer = hwBufPtr->createIndexBuffer(itype,
                                                   numIndices,
                                                   usage);
  hwIndexBuffer->writeData(0,
                           hwIndexBuffer->getSizeInBytes(),
                           indexBuffer,
                           true);

  auto indexData = std::make_unique<Ogre::IndexData>();
  indexData->indexBuffer = hwIndexBuffer;
  indexData->indexCount = numIndices;
  indexData->indexStart = 0;
  return indexData;
}

NifLoaderState::BoundedSubmesh
NifLoaderState::parseNiTriShape(nif::NiTriShape *block, LoadStatus &tag) {
  // NiTriShape blocks determine discrete pieces of geometry with a single
  // material and texture, and so translate to Ogre::SubMesh objects.
  // If a submesh with this name already exists, then it is either already
  // loaded or in the process of loading.
  Tagger tagger{tag};
  if (tag == LoadStatus::Loaded) {
    auto submesh = mesh->getSubMesh(block->name.str());
    if (submesh) {
      // TODO: How to get the bounding box once the submesh has been created?
      return {submesh, {}};
    } else {
      throw std::runtime_error(
          "NiTriShape marked as loaded but submesh does not exist");
    }
  }
  auto submesh = mesh->createSubMesh(block->name.str());

  submesh->useSharedVertices = false;

  // Here we have a slight problem with loading; For nif files, materials and
  // textures are independent and can be assigned independently to blocks. In
  // Ogre however, a texture has to have a parent material, and if two textures
  // have different parents then they must be different textures. This means
  // that we have to be much more careful about what being 'loaded' means, and
  // can only load a texture if there is a material to couple it to.
  // For simplicity, we will assume that textures and materials come in
  // unique pairs. TODO: Make textures independent of materials.
  auto taggedMaterialIt =
      std::find_if(block->properties.begin(),
                   block->properties.end(),
                   [&](const auto &property) {
                     // TODO: Check this is a valid reference
                     auto ref = static_cast<int32_t>(property);
                     auto ptr = blocks[ref].block.get();
                     return dynamic_cast<nif::NiMaterialProperty *>(ptr);
                   });
  auto taggedTexturingIt =
      std::find_if(block->properties.begin(),
                   block->properties.end(),
                   [&](const auto &property) {
                     // TODO: Check this is a valid reference
                     auto ref = static_cast<int32_t>(property);
                     auto ptr = blocks[ref].block.get();
                     return dynamic_cast<nif::NiTexturingProperty *>(ptr);
                   });

  if (taggedMaterialIt != block->properties.end()) {
    auto &taggedMaterial = blocks[static_cast<int32_t>(*taggedMaterialIt)];
    auto materialBlock =
        dynamic_cast<nif::NiMaterialProperty *>(taggedMaterial.block.get());
    auto &materialTag = taggedMaterial.tag;
    auto material = parseNiMaterialProperty(materialBlock, materialTag);
    auto pass = material->getTechnique(0)->getPass(0);

    submesh->setMaterialName(material->getName(), material->getGroup());

    if (taggedTexturingIt != block->properties.end()) {
      auto &taggedTexturing = blocks[static_cast<int32_t>(*taggedTexturingIt)];
      auto texturingBlock =
          dynamic_cast<nif::NiTexturingProperty *>(taggedTexturing.block.get());
      auto &texturingTag = taggedTexturing.tag;
      // We are assuming textures and materials come in unique pairs, so if this
      // texture has already been loaded, then it must have been attached to its
      // parent material already and so we can stop here. If we don't assume
      // this, then the material may end up with multiple copies of the same
      // texture overlaid, which causes visual problems.
      if (texturingTag == LoadStatus::Unloaded) {
        auto family = parseNiTexturingProperty(texturingBlock, texturingTag,
                                               pass);
        if (family.base) {
          pass->addTextureUnitState(family.base.release());
        }
        // TODO: Support more than base textures
      }
    }
  }

  // Ogre::SubMeshes cannot have transformations applied to them (that is
  // reserved for Ogre::SceneNodes), so we will apply it to all the vertex
  // information manually.
  Ogre::Vector3 translation{conversions::fromNif(block->translation)};
  Ogre::Matrix3 rotationMatrix{conversions::fromNif(block->rotation)};
  Ogre::Quaternion rotation{rotationMatrix};
  Ogre::Vector3 scale{block->scale, block->scale, block->scale};
  Ogre::Matrix4 transformation{};
  transformation.makeTransform(translation, scale, rotation);

  auto dataRef = static_cast<int32_t>(block->data);
  // TODO: Check this is a valid reference
  auto &taggedDataBlock = blocks[dataRef];
  auto data = dynamic_cast<nif::NiTriShapeData *>(taggedDataBlock.block.get());
  Tagger dataTagger{taggedDataBlock.tag};
  if (taggedDataBlock.tag == LoadStatus::Loaded) {
    return {submesh, getBoundingBox(data, transformation)};
  }

  auto vertexData = generateVertexData(data, transformation);
  auto indexData = generateIndexData(data);

  // Ogre::SubMesh leaves us to heap allocate the Ogre::VertexData but allocates
  // the Ogre::IndexData itself. Both have deleted copy-constructors so we can't
  // create index data on the stack and copy it over. Instead we do a Bad Thing,
  // deleting the submesh's indexData and replacing it with our own.
  delete submesh->indexData;

  // Transfer ownership to Ogre
  submesh->vertexData = vertexData.release();
  submesh->indexData = indexData.release();

  return {submesh, getBoundingBox(data, transformation)};
}

std::shared_ptr<Ogre::Material>
NifLoaderState::parseNiMaterialProperty(nif::NiMaterialProperty *block,
                                        LoadStatus &tag) {
  auto &materialManager = Ogre::MaterialManager::getSingleton();

  Tagger tagger{tag};
  if (tag == LoadStatus::Loaded) {
    auto material = materialManager.getByName(block->name.str(),
                                              mesh->getGroup());
    if (material) {
      return material;
    } else {
      throw std::runtime_error(
          "NiMaterialProperty marked as loaded but material does not exist");
    }
  }

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

  return material;
}

std::unique_ptr<Ogre::TextureUnitState>
NifLoaderState::parseTexDesc(nif::compound::TexDesc *tex,
                             Ogre::Pass *parent) {
  auto textureUnit = std::make_unique<Ogre::TextureUnitState>(parent);

  switch (tex->clampMode) {
    case nif::Enum::TexClampMode::CLAMP_S_CLAMP_T:
      textureUnit->setTextureAddressingMode(
          Ogre::TextureUnitState::TextureAddressingMode::TAM_CLAMP);
      break;
    case nif::Enum::TexClampMode::CLAMP_S_WRAP_T:
      textureUnit->setTextureAddressingMode(
          Ogre::TextureUnitState::TextureAddressingMode::TAM_CLAMP,
          Ogre::TextureUnitState::TextureAddressingMode::TAM_WRAP,
          Ogre::TextureUnitState::TextureAddressingMode::TAM_WRAP);
      break;
    case nif::Enum::TexClampMode::WRAP_S_CLAMP_T:
      textureUnit->setTextureAddressingMode(
          Ogre::TextureUnitState::TextureAddressingMode::TAM_WRAP,
          Ogre::TextureUnitState::TextureAddressingMode::TAM_CLAMP,
          Ogre::TextureUnitState::TextureAddressingMode::TAM_WRAP);
      break;
    case nif::Enum::TexClampMode::WRAP_S_WRAP_T:
      textureUnit->setTextureAddressingMode(
          Ogre::TextureUnitState::TextureAddressingMode::TAM_WRAP);
      break;
    default:
      textureUnit->setTextureAddressingMode(
          Ogre::TextureUnitState::TextureAddressingMode::TAM_WRAP);
      break;
  }

  switch (tex->filterMode) {
    case nif::Enum::TexFilterMode::FILTER_NEAREST:
      textureUnit->setTextureFiltering(
          Ogre::TextureFilterOptions::TFO_NONE);
      break;
    case nif::Enum::TexFilterMode::FILTER_BILERP:
      textureUnit->setTextureFiltering(
          Ogre::TextureFilterOptions::TFO_BILINEAR);
      break;
    case nif::Enum::TexFilterMode::FILTER_TRILERP:
      textureUnit->setTextureFiltering(
          Ogre::TextureFilterOptions::TFO_TRILINEAR);
      break;
    case nif::Enum::TexFilterMode::FILTER_NEAREST_MIPNEAREST:
      textureUnit->setTextureFiltering(
          Ogre::FilterOptions::FO_POINT,
          Ogre::FilterOptions::FO_POINT,
          Ogre::FilterOptions::FO_POINT);
      break;
    case nif::Enum::TexFilterMode::FILTER_NEAREST_MIPLERP:
      textureUnit->setTextureFiltering(
          Ogre::FilterOptions::FO_POINT,
          Ogre::FilterOptions::FO_POINT,
          Ogre::FilterOptions::FO_LINEAR);
      break;
    case nif::Enum::TexFilterMode::FILTER_BILERP_MIPNEAREST:
      textureUnit->setTextureFiltering(
          Ogre::FilterOptions::FO_LINEAR,
          Ogre::FilterOptions::FO_LINEAR,
          Ogre::FilterOptions::FO_POINT);
      break;
    case nif::Enum::TexFilterMode::FILTER_ANISOTROPIC:
      textureUnit->setTextureFiltering(
          Ogre::TextureFilterOptions::TFO_ANISOTROPIC);
      break;
    default:
      textureUnit->setTextureFiltering(
          Ogre::TextureFilterOptions::TFO_TRILINEAR);
      break;
  }

  textureUnit->setTextureCoordSet(tex->uvSet);

  if (tex->hasTextureTransform && *tex->hasTextureTransform) {
    auto &transform = *tex->textureTransform;

    Ogre::Matrix4 translation{};
    translation.makeTrans(transform.translation.u,
                          transform.translation.v,
                          0.0f);

    Ogre::Matrix4 scale{};
    scale.setScale(Ogre::Vector3{transform.scale.u, transform.scale.v, 1.0f});

    // TODO: Is transform.rotation really an anti-clockwise rotation in radians?
    Ogre::Matrix4 rotation{};
    rotation.makeTransform({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f},
                           Ogre::Quaternion{Ogre::Radian{transform.rotation},
                                            Ogre::Vector3::UNIT_Z});

    Ogre::Matrix4 center{};
    center.makeTrans(transform.center.u, transform.center.v, 0.0f);
    Ogre::Matrix4 centerInv = center.inverse();

    switch (transform.transformMethod) {
      case nif::Enum::TransformMethod::MayaDeprecated:
        textureUnit->setTextureTransform(
            center * rotation * centerInv * translation * scale);
        break;
      case nif::Enum::TransformMethod::Max:
        textureUnit->setTextureTransform(
            center * scale * rotation * translation * centerInv);
        break;
      case nif::Enum::TransformMethod::Maya: {
        Ogre::Matrix4 fromMaya{};
        fromMaya.makeTransform({0.0f, 1.0f, 0.0f},
                               {0.0f, -1.0f, 0.0f},
                               Ogre::Quaternion::ZERO);
        textureUnit->setTextureTransform(
            center * rotation * centerInv * fromMaya * translation * scale);
        break;
      }
      default:textureUnit->setTextureTransform(Ogre::Matrix4::IDENTITY);
        break;
    }
  }

  // TODO: Check this is a valid reference
  auto sourceRef = static_cast<int32_t>(tex->source);
  TaggedBlock taggedSource = blocks[sourceRef];
  LoadStatus &sourceTag = taggedSource.tag;
  auto source = dynamic_cast<nif::NiSourceTexture *>(taggedSource.block.get());
  parseNiSourceTexture(source, sourceTag, textureUnit.get());

  return textureUnit;
}

void NifLoaderState::parseNiSourceTexture(nif::NiSourceTexture *block,
                                          LoadStatus &tag,
                                          Ogre::TextureUnitState *tex) {
  Tagger tagger{tag};

  if (block->useExternal) {
    using ExternalTextureFile = nif::NiSourceTexture::ExternalTextureFile;
    auto &texFile = std::get<ExternalTextureFile>(block->textureFileData);
    std::string str = texFile.filename.string.str();
    // It is necessary to normalize the path because Ogre doesn't actually ask
    // the archive whether the file exists or not, it just checks against all
    // the filenames with a direct comparison.
    // TODO: Pull this into a function
    std::string filename(str);
    std::transform(str.begin(), str.end(), filename.begin(),
                   [](unsigned char c) {
                     return static_cast<unsigned char>(std::tolower(
                         c == '\\' ? '/' : c));
                   });
    // This actually looks up filename in the resource manager and complains if
    // the resource doesn't exist.
    tex->setTextureName(filename);
  } else {
    // We do not support internal textures, see InternalTextureFile comments
    logger.logMessage("Nif internal texture files are unsupported");
    return;
  }

  // Ogre is much more specific about its pixel layout than nif, and less
  // specific even still than dds. It is not obvious how many of these formats
  // translate, and some do not have analogues at all; Ogre does not seem to
  // support palettized textures, for instance. Moreover, it appears that this
  // is just setting a *preference*, so hopefully we are free to ignore it and
  // let Ogre decide.
  // TODO: Try and convert the pixel layout?

  switch (block->formatPrefs.mipMapFormat) {
    case nif::Enum::MipMapFormat::MIP_FMT_NO:tex->setNumMipmaps(0);
      break;
    case nif::Enum::MipMapFormat::MIP_FMT_YES:[[fallthrough]];
    case nif::Enum::MipMapFormat::MIP_FMT_DEFAULT:[[fallthrough]];
      // TODO: Use a global mipmap setting
    default:tex->setNumMipmaps(4);
      break;
  }

  // Ogre does not seem to provide control over this particular alpha setting

  // All textures are assumed static i.e. they cannot be dynamically changed
  // This can be set explicitly in a Texture, but not the TextureUnitState.

  // All textures are direct since we don't support internal textures
}

NifLoaderState::TextureFamily
NifLoaderState::parseNiTexturingProperty(nif::NiTexturingProperty *block,
                                         LoadStatus &tag, Ogre::Pass *pass) {
  Tagger tagger{tag};

  // Nif ApplyMode is for vertex colors, which are currently unsupported
  // TODO: Support ApplyMode with vertex colors

  TextureFamily family{};

  if (block->hasBaseTexture) {
    family.base = parseTexDesc(&block->baseTexture, pass);
  }
  if (block->hasDarkTexture) {
    family.dark = parseTexDesc(&block->darkTexture, pass);
  }
  if (block->hasDetailTexture) {
    family.detail = parseTexDesc(&block->detailTexture, pass);
  }
  if (block->hasGlossTexture) {
    family.gloss = parseTexDesc(&block->glossTexture, pass);
  }
  if (block->hasGlowTexture) {
    family.glow = parseTexDesc(&block->glowTexture, pass);
  }
  if (block->hasDecal0Texture) {
    family.decals.emplace_back(parseTexDesc(&block->decal0Texture, pass));
    if (block->hasDecal1Texture) {
      family.decals.emplace_back(parseTexDesc(&block->decal1Texture, pass));
      if (block->hasDecal2Texture) {
        family.decals.emplace_back(parseTexDesc(&block->decal2Texture, pass));
        if (block->hasDecal3Texture) {
          family.decals.emplace_back(parseTexDesc(&block->decal3Texture, pass));
        }
      }
    }
  }

  return family;
}

NifLoaderState::NifLoaderState(Ogre::Mesh *mesh,
                               NifLoader::BlockGraph untaggedBlocks)
    : mesh(mesh) {

  boost::copy_graph(untaggedBlocks, blocks);

  Ogre::AxisAlignedBox boundingBox{};

  for (const auto &vertex : blocks.vertex_set()) {
    auto taggedNiObject = blocks[vertex];
    const auto &niObject = taggedNiObject.block.get();
    auto &tag = taggedNiObject.tag;

    if (auto block = dynamic_cast<nif::NiTriShape *>(niObject)) {
      auto boundedSubmesh = parseNiTriShape(block, tag);
      boundingBox.merge(boundedSubmesh.bbox);
    } else if (auto block = dynamic_cast<nif::NiMaterialProperty *>(niObject)) {
      parseNiMaterialProperty(block, tag);
    }

    mesh->_setBounds(boundingBox);
  }
}

void NifLoader::loadResource(Ogre::Resource *resource) {
  auto mesh = dynamic_cast<Ogre::Mesh *>(resource);
  // TODO: Handle this properly
  assert(mesh != nullptr);

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

  for (const auto &v : vertices) {
    out << "v " << v.x << " " << v.y << " " << v.z << '\n';
  }

  for (const auto &n : normals) {
    out << "vn " << n.x << " " << n.y << " " << n.z << '\n';
  }

  for (const auto &uv : uvs) {
    out << "vt " << uv.x << " " << uv.y << "\n";
  }

  for (const auto &tri : tris) {
    out << "f " << tri[0] << '/' << tri[0] << '/' << tri[0]
        << " " << tri[1] << '/' << tri[1] << '/' << tri[1]
        << " " << tri[2] << '/' << tri[2] << '/' << tri[2] << '\n';
  }
}
} // namespace engine

