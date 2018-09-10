#include "engine/conversions.hpp"
#include "engine/nif_loader.hpp"
#include "engine/nif_loader_state.hpp"
#include "nif/enum.hpp"
#include "nif/niobject.hpp"
#include "nif/compound.hpp"
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/copy.hpp>
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
#include <OgreTextureUnitState.h>
#include <OgreVector3.h>
#include <OgreVector4.h>
#include <OgreVertexIndexData.h>
#include <algorithm>
#include <limits>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

namespace engine {

Ogre::AxisAlignedBox NifLoaderState::getBoundingBox(nif::NiGeometryData *block,
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

bool NifLoaderState::isWindingOrderCCW(Ogre::Vector3 v1, Ogre::Vector3 n1,
                                       Ogre::Vector3 v2, Ogre::Vector3 n2,
                                       Ogre::Vector3 v3, Ogre::Vector3 n3) {
  auto expected = (v2 - v1).crossProduct(v3 - v1);
  auto actual = (n1 + n2 + n3) / 3.0f;
  // Coordinate system is right-handed so this is positive for an
  // anticlockwise winding order and negative for a clockwise order.
  return expected.dotProduct(actual) > 0.0f;
}

long NifLoaderState::numCCWTriangles(nif::NiTriShapeData *block) {
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

std::unique_ptr<Ogre::VertexData>
NifLoaderState::generateVertexData(nif::NiGeometryData *block,
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
NifLoaderState::generateIndexData(nif::NiTriStripsData *block) {
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

NifLoaderState::BoundedSubmesh
NifLoaderState::parseNiTriBasedGeom(nif::NiTriBasedGeom *block,
                                    LoadStatus &tag) {
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
  if (dataRef < 0 || dataRef >= blocks.vertex_set().size()) {
    throw std::out_of_range("Nonexistent reference");
  }
  auto &taggedDataBlock = blocks[dataRef];
  auto dataBlock = taggedDataBlock.block.get();
  auto geometryData = dynamic_cast<nif::NiGeometryData *>(dataBlock);
  if (taggedDataBlock.tag == LoadStatus::Loaded) {
    auto bbox = getBoundingBox(geometryData, transformation);
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

  auto vertexData = generateVertexData(geometryData, transformation);

  // Ogre::SubMesh leaves us to heap allocate the Ogre::VertexData but allocates
  // the Ogre::IndexData itself. Both have deleted copy-constructors so we can't
  // create index data on the stack and copy it over. Instead we do a Bad Thing,
  // deleting the submesh's indexData and replacing it with our own.
  delete submesh->indexData;

  // Transfer ownership to Ogre
  submesh->vertexData = vertexData.release();
  submesh->indexData = indexData.release();

  return {submesh, getBoundingBox(geometryData, transformation)};
}

std::shared_ptr<Ogre::Material>
NifLoaderState::parseNiMaterialProperty(nif::NiMaterialProperty *block,
                                        LoadStatus &tag) {
  auto &materialManager = Ogre::MaterialManager::getSingleton();

  Tagger tagger{tag};
  // Nif materials are intended to have mesh-local names. We cannot guarantee
  // that the mesh's group is unique, so we prepend the mesh name.
  std::string meshName = mesh->getName();
  std::string materialName = meshName.append("\\").append(block->name.str());
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
  fsParams->setNamedConstant("diffuseMap", 0);
  fsParams->setNamedAutoConstant("lightPosition",
                                 AutoConst::ACT_LIGHT_POSITION, 0);

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

    if (auto niTriBasedGeom = dynamic_cast<nif::NiTriBasedGeom *>(niObject)) {
      auto[submesh, bbox] = parseNiTriBasedGeom(niTriBasedGeom, tag);
      boundingBox.merge(bbox);
    }

    mesh->_setBounds(boundingBox);
  }
}
} // namespace engine