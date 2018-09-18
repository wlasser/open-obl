#include "engine/conversions.hpp"
#include "engine/nif_loader.hpp"
#include "engine/nif_loader_state.hpp"
#include "nif/enum.hpp"
#include "nif/niobject.hpp"
#include "nif/compound.hpp"
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/copy.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <OgreAxisAlignedBox.h>
#include <OgreHardwareBufferManager.h>
#include <OgreHardwareIndexBuffer.h>
#include <OgreHardwareVertexBuffer.h>
#include <OgreLogManager.h>
#include <OgreMaterial.h>
#include <OgreMaterialManager.h>
#include <OgreMatrix3.h>
#include <OgreMatrix4.h>
#include <OgreMesh.h>
#include <OgrePass.h>
#include <OgreQuaternion.h>
#include <OgreSubMesh.h>
#include <OgreTechnique.h>
#include <OgreTextureManager.h>
#include <OgreTextureUnitState.h>
#include <OgreVector3.h>
#include <OgreVector4.h>
#include <OgreVertexIndexData.h>
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <limits>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

namespace engine::nifloader {

Tagger::Tagger(LoadStatus &tag) : tag(tag) {
  switch (tag) {
    case LoadStatus::Unloaded:tag = LoadStatus::Loading;
      break;
    case LoadStatus::Loading:
      throw std::runtime_error("Cycle detected while loading nif file");
    default: break;
  }
}

Tagger::~Tagger() {
  tag = LoadStatus::Loaded;
}

Ogre::AxisAlignedBox getBoundingBox(nif::NiGeometryData *block,
                                    Ogre::Matrix4 transformation) {
  using namespace conversions;
  const auto fltMin = std::numeric_limits<float>::lowest();
  const auto fltMax = std::numeric_limits<float>::max();
  Ogre::Vector3 bboxMin{fltMax, fltMax, fltMax};
  Ogre::Vector3 bboxMax{fltMin, fltMin, fltMin};

  if (!block->hasVertices || block->vertices.empty()) return {};

  for (const auto &vertex : block->vertices) {
    Ogre::Vector4 ogreV{fromBSCoordinates(fromNif(vertex))};
    auto v = transformation * ogreV;
    // NB: Cannot use else if, both branches apply if the mesh is flat
    if (v.x < bboxMin.x) bboxMin.x = v.x;
    if (v.x > bboxMax.x) bboxMax.x = v.x;
    if (v.y < bboxMin.y) bboxMin.y = v.y;
    if (v.y > bboxMax.y) bboxMax.y = v.y;
    if (v.z < bboxMin.z) bboxMin.z = v.z;
    if (v.z > bboxMax.z) bboxMax.z = v.z;
  }
  return {bboxMin, bboxMax};
}

bool isWindingOrderCCW(Ogre::Vector3 v1, Ogre::Vector3 n1,
                       Ogre::Vector3 v2, Ogre::Vector3 n2,
                       Ogre::Vector3 v3, Ogre::Vector3 n3) {
  auto expected = (v2 - v1).crossProduct(v3 - v1);
  auto actual = (n1 + n2 + n3) / 3.0f;
  // Coordinate system is right-handed so this is positive for an
  // anticlockwise winding order and negative for a clockwise order.
  return expected.dotProduct(actual) > 0.0f;
}

long numCCWTriangles(nif::NiTriShapeData *block) {
  assert(block->hasNormals);
  using conversions::fromNif;
  return std::count_if(block->triangles.begin(), block->triangles.end(),
                       [&](const auto &tri) {
                         return isWindingOrderCCW(
                             fromNif(block->vertices[tri.v1]),
                             fromNif(block->normals[tri.v1]),
                             fromNif(block->vertices[tri.v2]),
                             fromNif(block->normals[tri.v2]),
                             fromNif(block->vertices[tri.v3]),
                             fromNif(block->normals[tri.v3]));
                       });
}

std::filesystem::path toNormalMap(std::filesystem::path texFile) {
  auto extension = texFile.extension();
  texFile.replace_extension("");
  texFile += "_n";
  texFile += extension;
  return texFile;
}

Ogre::Matrix4 getTransform(nif::NiAVObject *block) {
  using namespace conversions;
  Ogre::Vector3 translation{fromBSCoordinates(fromNif(block->translation))};

  Ogre::Matrix3 rotationMatrix{fromBSCoordinates(fromNif(block->rotation))};
  Ogre::Quaternion rotation{rotationMatrix.transpose()};

  Ogre::Vector3 scale{block->scale, block->scale, block->scale};

  Ogre::Matrix4 trans{};
  trans.makeTransform(translation, scale, rotation);
  return trans;
}

std::unique_ptr<Ogre::VertexData>
LoaderState::generateVertexData(nif::NiGeometryData *block,
                                Ogre::Matrix4 transformation,
                                std::vector<nif::compound::Vector3> *bitangents,
                                std::vector<nif::compound::Vector3> *tangents) {
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
  if (bitangents) {
    vertDecl->addElement(source, bytesPerVertex, Ogre::VET_FLOAT3,
                         Ogre::VES_BINORMAL);
    bytesPerVertex += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
    offset += 3;
  }
  if (tangents) {
    vertDecl->addElement(source, bytesPerVertex, Ogre::VET_FLOAT3,
                         Ogre::VES_TANGENT);
    bytesPerVertex += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
    offset += 3;
  }

  // Normal vectors are not translated and transform with the inverse
  // transpose of the transformation matrix. We will also need this for tangents
  // and bitangents so we compute it now.
  auto normalTransformation = transformation;
  normalTransformation.setTrans(Ogre::Vector3::ZERO);
  normalTransformation = normalTransformation.inverse().transpose();

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
      using namespace conversions;
      Ogre::Vector4 ogreV{fromBSCoordinates(fromNif(vertex))};
      auto v = transformation * ogreV;
      *it = v.x;
      *(it + 1) = v.y;
      *(it + 2) = v.z;
      it += offset;
    }
    localOffset += 3;
  }
  // TODO: Blend weights
  if (block->hasNormals) {
    auto it = vertexBuffer.begin() + localOffset;
    for (const auto &normal : block->normals) {
      using namespace conversions;
      Ogre::Vector4 ogreN{fromBSCoordinates(fromNif(normal))};
      auto n = normalTransformation * ogreN;
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
  if (bitangents) {
    auto it = vertexBuffer.begin() + localOffset;
    for (const auto &bitangent : *bitangents) {
      using namespace conversions;
      Ogre::Vector4 ogreBt{fromBSCoordinates(fromNif(bitangent))};
      auto bt = normalTransformation * ogreBt;
      *it = bt.x;
      *(it + 1) = bt.y;
      *(it + 2) = bt.z;
      it += offset;
    }
    localOffset += 3;
  }
  if (tangents) {
    auto it = vertexBuffer.begin() + localOffset;
    for (const auto &tangent : *tangents) {
      using namespace conversions;
      Ogre::Vector4 ogreT{fromBSCoordinates(fromNif(tangent))};
      auto t = normalTransformation * ogreT;
      *it = t.x;
      *(it + 1) = t.y;
      *(it + 2) = t.z;
      it += offset;
    }
    localOffset += 3;
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
LoaderState::generateIndexData(nif::NiTriShapeData *block) {
  auto hwBufPtr = Ogre::HardwareBufferManager::getSingletonPtr();

  // We can assume that compound::Triangle has no padding and std::vector
  // is sequential, so can avoid copying the faces.
  auto indexBuffer = reinterpret_cast<uint16_t *>(block->triangles.data());

  auto usage = Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY;
  auto itype = Ogre::HardwareIndexBuffer::IT_16BIT;
  std::size_t numIndices = 3u * block->numTriangles;

  // Copy the triangle (index) buffer into a hardware buffer.
  auto hwIndexBuffer = hwBufPtr->createIndexBuffer(itype, numIndices, usage);
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

std::unique_ptr<Ogre::IndexData>
LoaderState::generateIndexData(nif::NiTriStripsData *block) {
  auto hwBufPtr = Ogre::HardwareBufferManager::getSingletonPtr();

  auto usage = Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY;
  auto itype = Ogre::HardwareIndexBuffer::IT_16BIT;
  std::size_t numIndices = std::accumulate(block->stripLengths.begin(),
                                           block->stripLengths.end(), 0u);

  auto hwIndexBuffer = hwBufPtr->createIndexBuffer(itype, numIndices, usage);
  std::size_t offset = 0;
  for (const auto &strip : block->points) {
    hwIndexBuffer->writeData(offset, 2u * strip.size(), &strip[0], true);
    offset += 2u * strip.size();
  }

  auto indexData = std::make_unique<Ogre::IndexData>();
  indexData->indexBuffer = hwIndexBuffer;
  indexData->indexCount = numIndices;
  indexData->indexStart = 0;
  return indexData;
}

BoundedSubmesh
LoaderState::parseNiTriBasedGeom(nif::NiTriBasedGeom *block,
                                 LoadStatus &tag,
                                 const Ogre::Matrix4 &transform) {
  // NiTriBasedGeom blocks determine discrete pieces of geometry with a single
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
          "NiTriBasedGeom marked as loaded but submesh does not exist");
    }
  }
  auto submesh = mesh->createSubMesh(block->name.str());

  submesh->useSharedVertices = false;

  std::vector<nif::compound::Vector3> bitangents{};
  std::vector<nif::compound::Vector3> tangents{};

  // For normal mapping we need tangent and bitangent information given inside
  // an NiBinaryExtraData block. For low versions, the extra data is arranged
  // like a linked list, and for high versions it's an array.
  // TODO: Support the linked list version
  if (block->extraDataArray) {
    auto xtraIt = block->extraDataArray->begin();
    do {
      xtraIt = std::find_if(xtraIt, block->extraDataArray->end(),
                            [this](const auto &xtraData) {
                              return getBlock<nif::NiBinaryExtraData>(xtraData);
                            });
      if (xtraIt == block->extraDataArray->end()) break;

      auto taggedXtraData = blocks[static_cast<int32_t>(*xtraIt)];
      auto xtraData = dynamic_cast<nif::NiBinaryExtraData *>(
          taggedXtraData.block.get());
      auto &xtraDataTag = taggedXtraData.tag;

      if (xtraData->name) {
        std::string name = xtraData->name->str();
        // Tangent data should begin with "Tangent space"
        if (name.find("Tangent space") == 0) {
          // Format seems to be t1 t2 t3 ... b1 b2 b3 ...
          // We need both the number of bytes in each list, and the number of
          // vertices
          std::size_t numBytes = xtraData->data.dataSize / 2;
          std::size_t numVertices = numBytes / (3 * sizeof(float));
          bitangents.resize(numVertices);
          tangents.resize(numVertices);
          // Poor naming choices, maybe?
          std::memcpy(bitangents.data(), xtraData->data.data.data() + numBytes,
                      numBytes);
          std::memcpy(tangents.data(), xtraData->data.data.data(), numBytes);
        }
      }
    } while (++xtraIt != block->extraDataArray->end());
  }

  // Here we have a slight problem with loading; For nif files, materials and
  // textures are independent and can be assigned independently to blocks. In
  // Ogre however, a texture has to have a parent material, and if two textures
  // have different parents then they must be different textures. This means
  // that we have to be much more careful about what being 'loaded' means, and
  // can only load a texture if there is a material to couple it to.
  // For simplicity, we assume that the same material will not be given
  // multiple textures in the same mesh. Multiple materials are allowed to share
  // the same texture, however. This is only reasonable because materials are
  // mesh-local.

  // Find the material property, if any
  auto taggedMatIt =
      std::find_if(block->properties.begin(), block->properties.end(),
                   [this](const auto &property) {
                     return getBlock<nif::NiMaterialProperty>(property);
                   });

  // Find the texturing property, if any
  auto taggedTexIt =
      std::find_if(block->properties.begin(), block->properties.end(),
                   [this](const auto &property) {
                     return getBlock<nif::NiTexturingProperty>(property);
                   });

  // It is ok to have a material but no texture
  if (taggedMatIt != block->properties.end()) {
    auto &taggedMat = blocks[static_cast<int32_t>(*taggedMatIt)];
    auto matBlock =
        dynamic_cast<nif::NiMaterialProperty *>(taggedMat.block.get());
    auto &matTag = taggedMat.tag;

    // If the material has already been loaded, then by the assumption that each
    // material has a unique texture (if any), the associated texture must
    // already have been loaded and attached to the material. We can load the
    // material (presumably from a cache) and skip the texture loading.
    // Without this assumption we would have to clone the material and attach
    // the new texture.
    if (matTag == LoadStatus::Loaded) {
      auto material = parseNiMaterialProperty(matBlock, matTag);
      submesh->setMaterialName(material->getName(), material->getGroup());
    } else {
      auto material = parseNiMaterialProperty(matBlock, matTag);
      submesh->setMaterialName(material->getName(), material->getGroup());
      auto pass = material->getTechnique(0)->getPass(0);

      // The texture may or may not have already been loaded, but it has
      // definitely not been attached to the material.
      if (taggedTexIt != block->properties.end()) {
        auto &taggedTex = blocks[static_cast<int32_t>(*taggedTexIt)];
        auto texBlock =
            dynamic_cast<nif::NiTexturingProperty *>(taggedTex.block.get());
        auto &texTag = taggedTex.tag;
        auto family = parseNiTexturingProperty(texBlock, texTag, pass);
        if (family.base) {
          pass->addTextureUnitState(family.base.release());
          if (family.normal) {
            pass->addTextureUnitState(family.normal.release());
          }
        }
        // TODO: Support more than base textures
      }
    }
  }

  // Ogre::SubMeshes cannot have transformations applied to them (that is
  // reserved for Ogre::SceneNodes), so we will apply it to all the vertex
  // information manually.
  auto totalTrans = transform * getTransform(block);

  auto dataRef = static_cast<int32_t>(block->data);
  if (dataRef < 0 || dataRef >= blocks.vertex_set().size()) {
    throw std::out_of_range("Nonexistent reference");
  }
  auto &taggedDataBlock = blocks[dataRef];
  auto dataBlock = taggedDataBlock.block.get();
  auto geometryData = dynamic_cast<nif::NiGeometryData *>(dataBlock);
  if (taggedDataBlock.tag == LoadStatus::Loaded) {
    auto bbox = getBoundingBox(geometryData, totalTrans);
    return {submesh, bbox};
  }

  std::unique_ptr<Ogre::IndexData> indexData{};
  Tagger dataTagger{taggedDataBlock.tag};

  // TODO: Replace this with a virtual function somehow?
  if (auto triShapeData = dynamic_cast<nif::NiTriShapeData *>(dataBlock)) {
    indexData = generateIndexData(triShapeData);
    submesh->operationType = Ogre::RenderOperation::OT_TRIANGLE_LIST;
  } else if (auto triStripsData =
      dynamic_cast<nif::NiTriStripsData *>(dataBlock)) {
    indexData = generateIndexData(triStripsData);
    submesh->operationType = Ogre::RenderOperation::OT_TRIANGLE_STRIP;
  }

  auto vertexData = generateVertexData(geometryData, totalTrans,
                                       &bitangents, &tangents);

  // Ogre::SubMesh leaves us to heap allocate the Ogre::VertexData but allocates
  // the Ogre::IndexData itself. Both have deleted copy-constructors so we can't
  // create index data on the stack and copy it over. Instead we do a Bad Thing,
  // deleting the submesh's indexData and replacing it with our own.
  delete submesh->indexData;

  // Transfer ownership to Ogre
  submesh->vertexData = vertexData.release();
  submesh->indexData = indexData.release();

  return {submesh, getBoundingBox(geometryData, totalTrans)};
}

std::shared_ptr<Ogre::Material>
LoaderState::parseNiMaterialProperty(nif::NiMaterialProperty *block,
                                     LoadStatus &tag) {
  auto &materialManager = Ogre::MaterialManager::getSingleton();

  Tagger tagger{tag};
  // Materials should be nif local, so a reasonable strategy would be name the
  // Ogre::Material by the mesh name followed by the nif material name.
  // Unfortunately, nif material names are not necessarily unique, even within
  // a nif file. We therefore resort to using the block index.
  // TODO: This is way more work than we need to do here
  auto it = std::find_if(blocks.vertex_set().begin(), blocks.vertex_set().end(),
                         [this, &block](auto i) {
                           return blocks[i].block.get() == block;
                         });

  std::string meshName = mesh->getName();
  std::string materialName = meshName.append("/").append(std::to_string(*it));
  if (tag == LoadStatus::Loaded) {
    auto material = materialManager.getByName(materialName, mesh->getGroup());
    if (material) {
      return material;
    } else {
      throw std::runtime_error(
          "NiMaterialProperty marked as loaded but material does not exist");
    }
  }

  auto material = materialManager.create(materialName, mesh->getGroup());

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

  using AutoConst = Ogre::GpuProgramParameters::AutoConstantType;
  pass->setVertexProgram("genericMaterial_vs_glsl", true);
  auto vsParams = pass->getVertexProgramParameters();
  vsParams->setNamedAutoConstant("world",
                                 AutoConst::ACT_WORLD_MATRIX);
  vsParams->setNamedAutoConstant("worldInverseTranspose",
                                 AutoConst::ACT_INVERSE_TRANSPOSE_WORLD_MATRIX);
  vsParams->setNamedAutoConstant("worldViewProj",
                                 AutoConst::ACT_WORLDVIEWPROJ_MATRIX);
  vsParams->setNamedAutoConstant("viewPos",
                                 AutoConst::ACT_CAMERA_POSITION);

  pass->setFragmentProgram("genericMaterial_fs_glsl", true);
  auto fsParams = pass->getFragmentProgramParameters();
  const int numLights = 8;
  fsParams->setNamedConstant("diffuseMap", 0);
  fsParams->setNamedConstant("normalMap", 1);
  fsParams->setNamedAutoConstant("lightPositionArray",
                                 AutoConst::ACT_LIGHT_POSITION_ARRAY,
                                 numLights);
  fsParams->setNamedAutoConstant("lightDiffuseArray",
                                 AutoConst::ACT_LIGHT_DIFFUSE_COLOUR_ARRAY,
                                 numLights);
  fsParams->setNamedAutoConstant("lightAttenuationArray",
                                 AutoConst::ACT_LIGHT_ATTENUATION_ARRAY,
                                 numLights);
  fsParams->setNamedAutoConstant("matShininess",
                                 AutoConst::ACT_SURFACE_SHININESS);
  fsParams->setNamedAutoConstant("matDiffuse",
                                 AutoConst::ACT_SURFACE_DIFFUSE_COLOUR);
  fsParams->setNamedAutoConstant("matSpecular",
                                 AutoConst::ACT_SURFACE_SPECULAR_COLOUR);

  return material;
}

std::unique_ptr<Ogre::TextureUnitState>
LoaderState::parseTexDesc(nif::compound::TexDesc *tex,
                          Ogre::Pass *parent,
                          const std::optional<std::string> &textureOverride) {
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
  parseNiSourceTexture(source, sourceTag, textureUnit.get(), textureOverride);

  return textureUnit;
}

void LoaderState::parseNiSourceTexture(nif::NiSourceTexture *block,
                                       LoadStatus &tag,
                                       Ogre::TextureUnitState *tex,
                                       const std::optional<std::string> &textureOverride) {
  Tagger tagger{tag};

  if (block->useExternal) {
    using ExternalTextureFile = nif::NiSourceTexture::ExternalTextureFile;
    auto &texFile = std::get<ExternalTextureFile>(block->textureFileData);
    if (textureOverride) {
      tex->setTextureName(*textureOverride);
    } else {
      tex->setTextureName(conversions::normalizePath(
          texFile.filename.string.str()));
    }
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

TextureFamily
LoaderState::parseNiTexturingProperty(nif::NiTexturingProperty *block,
                                      LoadStatus &tag, Ogre::Pass *pass) {
  Tagger tagger{tag};

  // Nif ApplyMode is for vertex colors, which are currently unsupported
  // TODO: Support ApplyMode with vertex colors

  TextureFamily family{};

  if (block->hasBaseTexture) {
    family.base = parseTexDesc(&block->baseTexture, pass);
    // Normal mapping is automatically turned on if a normal map exists. If one
    // doesn't exist, then we use a flat normal map.
    auto &texMgr = Ogre::TextureManager::getSingleton();
    if (auto normalMap = toNormalMap(family.base->getTextureName());
        texMgr.resourceExists(normalMap, pass->getResourceGroup())) {
      family.normal = parseTexDesc(&block->baseTexture, pass, normalMap);
    } else {
      family.normal = parseTexDesc(&block->baseTexture, pass,
                                   "textures/flat_n.dds");
    }
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

LoaderState::LoaderState(Ogre::Mesh *mesh, BlockGraph untaggedBlocks)
    : mesh(mesh) {
  boost::copy_graph(untaggedBlocks, blocks);

  std::vector<boost::default_color_type> colorMap(boost::num_vertices(blocks));
  auto propertyMap = boost::make_iterator_property_map(
      colorMap.begin(), boost::get(boost::vertex_index, blocks));

  boost::depth_first_search(blocks, TBGVisitor(*this), propertyMap);
}

// This is a new connected component so we need to reset the transformation to
// the identity. NB: This vertex will still be discovered so setting the
// transformation to the vertex's will result in it being applied twice.
void TBGVisitor::start_vertex(vertex_descriptor v, const Graph &g) {
  transform = Ogre::Matrix4::IDENTITY;
}

// If this vertex corresponds to a geometry block, then load it with the current
// transformation. If it's an NiNode, update the current transformation so that
// this child transformation occurs before any parent ones.
void TBGVisitor::discover_vertex(vertex_descriptor v, const Graph &g) {
  auto &taggedNiObject = g[v];
  auto *niObject = taggedNiObject.block.get();
  auto &tag = taggedNiObject.tag;

  if (auto niTriBasedGeom = dynamic_cast<nif::NiTriBasedGeom *>(niObject)) {
    auto[submesh, subBbox] = state.parseNiTriBasedGeom(niTriBasedGeom, tag,
                                                       transform);
    auto bbox = state.mesh->getBounds();
    bbox.merge(subBbox);
    state.mesh->_setBounds(bbox);
  } else if (auto niNode = dynamic_cast<nif::NiNode *>(niObject)) {
    transform = transform * getTransform(niNode);
  }
}

// If we have finished reading an NiNode, then we can remove its transformation.
void TBGVisitor::finish_vertex(vertex_descriptor v, const Graph &g) {
  auto *niObject = g[v].block.get();
  if (auto niNode = dynamic_cast<nif::NiNode *>(niObject)) {
    transform = transform * getTransform(niNode).inverse();
  }
}

} // namespace engine::nifloader