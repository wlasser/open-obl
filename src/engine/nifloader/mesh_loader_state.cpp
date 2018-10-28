#include "engine/conversions.hpp"
#include "engine/nifloader/loader_state.hpp"
#include "engine/nifloader/mesh_loader_state.hpp"
#include <boost/graph/copy.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <algorithm>
#include <numeric>

namespace engine::nifloader {

Ogre::AxisAlignedBox getBoundingBox(const nif::NiGeometryData &block,
                                    Ogre::Matrix4 transformation) {
  using namespace conversions;
  const auto fltMin = std::numeric_limits<float>::lowest();
  const auto fltMax = std::numeric_limits<float>::max();
  Ogre::Vector3 bboxMin{fltMax, fltMax, fltMax};
  Ogre::Vector3 bboxMax{fltMin, fltMin, fltMin};

  if (!block.hasVertices || block.vertices.empty()) return {};

  for (const auto &vertex : block.vertices) {
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

long numCCWTriangles(const nif::NiTriShapeData &block) {
  assert(block.hasNormals);
  using conversions::fromNif;
  return std::count_if(block.triangles.begin(), block.triangles.end(),
                       [&](const auto &tri) {
                         return isWindingOrderCCW(
                             fromNif(block.vertices[tri.v1]),
                             fromNif(block.normals[tri.v1]),
                             fromNif(block.vertices[tri.v2]),
                             fromNif(block.normals[tri.v2]),
                             fromNif(block.vertices[tri.v3]),
                             fromNif(block.normals[tri.v3]));
                       });
}

std::filesystem::path toNormalMap(std::filesystem::path texFile) {
  auto extension = texFile.extension();
  texFile.replace_extension("");
  texFile += "_n";
  texFile += extension;
  return texFile;
}

std::unique_ptr<Ogre::VertexData>
generateVertexData(const nif::NiGeometryData &block,
                   Ogre::Matrix4 transformation,
                   std::vector<nif::compound::Vector3> *bitangents,
                   std::vector<nif::compound::Vector3> *tangents) {
  // Ogre expects a heap allocated raw pointer, but to improve exception safety
  // we construct an unique_ptr then relinquish control of it to Ogre.
  auto vertexData{std::make_unique<Ogre::VertexData>()};
  vertexData->vertexCount = block.numVertices;
  auto vertDecl{vertexData->vertexDeclaration};
  auto vertBind{vertexData->vertexBufferBinding};
  auto *hwBufMgr{Ogre::HardwareBufferManager::getSingletonPtr()};

  // Specify the order of data in the vertex buffer. This is per vertex,
  // so the vertices, normals etc will have to be interleaved in the buffer.
  std::size_t offset{0};
  std::size_t bytesPerVertex{0};
  const unsigned short source{0};

  // Vertices
  vertDecl->addElement(source, bytesPerVertex, Ogre::VET_FLOAT3,
                       Ogre::VES_POSITION);
  bytesPerVertex += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
  offset += 3;

  // TODO: Blend weights

  // Normals
  vertDecl->addElement(source, bytesPerVertex, Ogre::VET_FLOAT3,
                       Ogre::VES_NORMAL);
  bytesPerVertex += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
  offset += 3;

  // Vertex colours
  vertDecl->addElement(source, bytesPerVertex, Ogre::VET_FLOAT3,
                       Ogre::VES_DIFFUSE);
  bytesPerVertex += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
  offset += 3;

  // UVs
  vertDecl->addElement(source, bytesPerVertex, Ogre::VET_FLOAT2,
                       Ogre::VES_TEXTURE_COORDINATES, 0);
  bytesPerVertex += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT2);
  offset += 2;

  // Bitangents
  vertDecl->addElement(source, bytesPerVertex, Ogre::VET_FLOAT3,
                       Ogre::VES_BINORMAL);
  bytesPerVertex += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
  offset += 3;

  // Tangents
  vertDecl->addElement(source, bytesPerVertex, Ogre::VET_FLOAT3,
                       Ogre::VES_TANGENT);
  bytesPerVertex += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
  offset += 3;

  // Normal vectors are not translated and transform with the inverse
  // transpose of the transformation matrix. We will also need this for tangents
  // and bitangents so we compute it now.
  auto normalTransformation{transformation};
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
  std::vector<float> vertexBuffer(offset * block.numVertices);
  std::size_t localOffset{0};

  // Vertices
  if (block.hasVertices) {
    auto it = vertexBuffer.begin();
    for (const auto &vertex : block.vertices) {
      using namespace conversions;
      const Ogre::Vector4 ogreV{fromBSCoordinates(fromNif(vertex))};
      const auto v{transformation * ogreV};
      *it = v.x;
      *(it + 1) = v.y;
      *(it + 2) = v.z;
      it += offset;
    }
    localOffset += 3;
  } else {
    throw std::runtime_error("NiGeometryData has no vertices");
  }

  // TODO: Blend weights

  // Normals
  if (block.hasNormals) {
    auto it = vertexBuffer.begin() + localOffset;
    for (const auto &normal : block.normals) {
      using namespace conversions;
      const Ogre::Vector4 ogreN{fromBSCoordinates(fromNif(normal))};
      const auto n{normalTransformation * ogreN};
      *it = n.x;
      *(it + 1) = n.y;
      *(it + 2) = n.z;
      it += offset;
    }
    localOffset += 3;
  } else {
    throw std::runtime_error("NiGeometryData has no normals");
  }

  // Vertex colours
  if (block.hasVertexColors) {
    auto it = vertexBuffer.begin() + localOffset;
    for (const auto &col : block.vertexColors) {
      *it = col.r;
      *(it + 1) = col.g;
      *(it + 2) = col.b;
      it += offset;
    }
    localOffset += 3;
  } else {
    // If a mesh doesn't have any vertex colours then we default to white
    auto it = vertexBuffer.begin() + localOffset;
    for (int i = 0; i < block.numVertices; ++i) {
      std::fill(it, it + 3, 1.0f);
      it += offset;
    }
    localOffset += 3;
  }

  // UVs
  if (!block.uvSets.empty()) {
    auto it = vertexBuffer.begin() + localOffset;
    // TODO: Support more than one UV set?
    for (const auto &uv : block.uvSets[0]) {
      *it = uv.u;
      *(it + 1) = uv.v;
      it += offset;
    }
    localOffset += 2;
  }

  // Bitangents
  if (bitangents) {
    auto it = vertexBuffer.begin() + localOffset;
    for (const auto &bitangent : *bitangents) {
      using namespace conversions;
      const Ogre::Vector4 ogreBt{fromBSCoordinates(fromNif(bitangent))};
      const auto bt{normalTransformation * ogreBt};
      *it = bt.x;
      *(it + 1) = bt.y;
      *(it + 2) = bt.z;
      it += offset;
    }
    localOffset += 3;
  } else {
    // TODO: Compute the bitangents if they don't exist
  }

  // Tangents
  if (tangents) {
    auto it = vertexBuffer.begin() + localOffset;
    for (const auto &tangent : *tangents) {
      using namespace conversions;
      const Ogre::Vector4 ogreT{fromBSCoordinates(fromNif(tangent))};
      const auto t{normalTransformation * ogreT};
      *it = t.x;
      *(it + 1) = t.y;
      *(it + 2) = t.z;
      it += offset;
    }
    localOffset += 3;
  } else {
    // TODO: Compute the tangents if they don't exist
  }

  // Copy the vertex buffer into a hardware buffer, and link the buffer to
  // the vertex declaration.
  const auto usage{Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY};
  const auto bpv{bytesPerVertex};

  auto hwBuf{hwBufMgr->createVertexBuffer(bpv, block.numVertices, usage)};
  hwBuf->writeData(0, hwBuf->getSizeInBytes(), vertexBuffer.data(), true);

  vertBind->setBinding(source, hwBuf);

  return vertexData;
}

std::unique_ptr<Ogre::IndexData>
generateIndexData(const nif::NiTriShapeData &block) {
  auto *hwBufMgr{Ogre::HardwareBufferManager::getSingletonPtr()};

  // We can assume that compound::Triangle has no padding and std::vector
  // is sequential, so can avoid copying the faces.
  auto indexBuffer{reinterpret_cast<const uint16_t *>(block.triangles.data())};

  const auto usage{Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY};
  const auto itype{Ogre::HardwareIndexBuffer::IT_16BIT};
  const std::size_t numIndices{3u * block.numTriangles};

  // Copy the triangle (index) buffer into a hardware buffer.
  auto hwBuf{hwBufMgr->createIndexBuffer(itype, numIndices, usage)};
  hwBuf->writeData(0, hwBuf->getSizeInBytes(), indexBuffer, true);

  auto indexData{std::make_unique<Ogre::IndexData>()};
  indexData->indexBuffer = hwBuf;
  indexData->indexCount = numIndices;
  indexData->indexStart = 0;
  return indexData;
}

std::unique_ptr<Ogre::IndexData>
generateIndexData(const nif::NiTriStripsData &block) {
  auto *hwBufMgr = Ogre::HardwareBufferManager::getSingletonPtr();

  const auto usage{Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY};
  const auto itype{Ogre::HardwareIndexBuffer::IT_16BIT};
  const std::size_t numIndices{std::accumulate(block.stripLengths.begin(),
                                               block.stripLengths.end(), 0u)};

  std::vector<uint16_t> buf{};
  buf.reserve(numIndices);
  for (const auto &strip : block.points) {
    buf.insert(buf.end(), strip.begin(), strip.end());
  }

  auto hwBuf{hwBufMgr->createIndexBuffer(itype, numIndices, usage)};
  hwBuf->writeData(0, hwBuf->getSizeInBytes(), buf.data(), true);

  auto indexData{std::make_unique<Ogre::IndexData>()};
  indexData->indexBuffer = hwBuf;
  indexData->indexCount = numIndices;
  indexData->indexStart = 0;
  return indexData;
}

TextureFamily
MeshLoaderState::parseNiTexturingProperty(const nif::NiTexturingProperty &block,
                                          LoadStatus &tag, Ogre::Pass *pass) {
  Tagger tagger{tag};

  // Nif ApplyMode is for vertex colors, which are currently unsupported
  // TODO: Support ApplyMode with vertex colors

  TextureFamily family{};

  if (block.hasBaseTexture) {
    family.base = parseTexDesc(&block.baseTexture, pass);
    // Normal mapping is automatically turned on if a normal map exists. If one
    // doesn't exist, then we use a flat normal map.
    auto &texMgr{Ogre::TextureManager::getSingleton()};
    if (auto normalMap = toNormalMap(family.base->getTextureName());
        texMgr.resourceExists(normalMap, pass->getResourceGroup())) {
      family.normal = parseTexDesc(&block.baseTexture, pass, normalMap);
    } else {
      family.normal = parseTexDesc(&block.baseTexture, pass,
                                   "textures/flat_n.dds");
    }
  }
  if (block.hasDarkTexture) {
    family.dark = parseTexDesc(&block.darkTexture, pass);
  }
  if (block.hasDetailTexture) {
    family.detail = parseTexDesc(&block.detailTexture, pass);
  }
  if (block.hasGlossTexture) {
    family.gloss = parseTexDesc(&block.glossTexture, pass);
  }
  if (block.hasGlowTexture) {
    family.glow = parseTexDesc(&block.glowTexture, pass);
  }
  if (block.hasDecal0Texture) {
    family.decals.emplace_back(parseTexDesc(&block.decal0Texture, pass));
    if (block.hasDecal1Texture) {
      family.decals.emplace_back(parseTexDesc(&block.decal1Texture, pass));
      if (block.hasDecal2Texture) {
        family.decals.emplace_back(parseTexDesc(&block.decal2Texture, pass));
        if (block.hasDecal3Texture) {
          family.decals.emplace_back(parseTexDesc(&block.decal3Texture, pass));
        }
      }
    }
  }

  return family;
}

void setSourceTexture(const nif::NiSourceTexture &block,
                      Ogre::TextureUnitState *tex,
                      const std::optional<std::string> &textureOverride) {
  if (block.useExternal) {
    if (textureOverride) {
      tex->setTextureName(*textureOverride);
    } else {
      using ExternalTextureFile = nif::NiSourceTexture::ExternalTextureFile;
      auto &texFile{std::get<ExternalTextureFile>(block.textureFileData)};
      // TODO: Use fs not std::fs
      tex->setTextureName(conversions::normalizePath(
          texFile.filename.string.str()));
    }
  } else {
    // We do not support internal textures, see InternalTextureFile comments
    return;
  }

  // Ogre is much more specific about its pixel layout than nif, and less
  // specific even still than dds. It is not obvious how many of these formats
  // translate, and some do not have analogues at all; Ogre does not seem to
  // support palettized textures, for instance. Moreover, it appears that this
  // is just setting a *preference*, so hopefully we are free to ignore it and
  // let Ogre decide.
  // TODO: Try and convert the pixel layout?

  switch (block.formatPrefs.mipMapFormat) {
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

void setClampMode(nif::Enum::TexClampMode mode, Ogre::TextureUnitState *tex) {
  using ClampMode = nif::Enum::TexClampMode;
  using AddressingMode = Ogre::TextureUnitState::TextureAddressingMode;

  switch (mode) {
    case ClampMode::CLAMP_S_CLAMP_T:
      tex->setTextureAddressingMode(AddressingMode::TAM_CLAMP);
      return;
    case ClampMode::CLAMP_S_WRAP_T:
      tex->setTextureAddressingMode(AddressingMode::TAM_CLAMP,
                                    AddressingMode::TAM_WRAP,
                                    AddressingMode::TAM_WRAP);
      return;
    case ClampMode::WRAP_S_CLAMP_T:
      tex->setTextureAddressingMode(AddressingMode::TAM_WRAP,
                                    AddressingMode::TAM_CLAMP,
                                    AddressingMode::TAM_WRAP);
      return;
    case ClampMode::WRAP_S_WRAP_T:
      tex->setTextureAddressingMode(AddressingMode::TAM_WRAP);
      return;
    default:tex->setTextureAddressingMode(AddressingMode::TAM_WRAP);
      return;
  }
}

void setFilterMode(nif::Enum::TexFilterMode mode, Ogre::TextureUnitState *tex) {
  using FilterMode = nif::Enum::TexFilterMode;

  switch (mode) {
    case FilterMode::FILTER_NEAREST:
      tex->setTextureFiltering(Ogre::TextureFilterOptions::TFO_NONE);
      return;
    case FilterMode::FILTER_BILERP:
      tex->setTextureFiltering(Ogre::TextureFilterOptions::TFO_BILINEAR);
      return;
    case FilterMode::FILTER_TRILERP:
      tex->setTextureFiltering(Ogre::TextureFilterOptions::TFO_TRILINEAR);
      return;
    case FilterMode::FILTER_NEAREST_MIPNEAREST:
      tex->setTextureFiltering(Ogre::FilterOptions::FO_POINT,
                               Ogre::FilterOptions::FO_POINT,
                               Ogre::FilterOptions::FO_POINT);
      return;
    case FilterMode::FILTER_NEAREST_MIPLERP:
      tex->setTextureFiltering(Ogre::FilterOptions::FO_POINT,
                               Ogre::FilterOptions::FO_POINT,
                               Ogre::FilterOptions::FO_LINEAR);
      return;
    case FilterMode::FILTER_BILERP_MIPNEAREST:
      tex->setTextureFiltering(Ogre::FilterOptions::FO_LINEAR,
                               Ogre::FilterOptions::FO_LINEAR,
                               Ogre::FilterOptions::FO_POINT);
      return;
    case FilterMode::FILTER_ANISOTROPIC:
      tex->setTextureFiltering(Ogre::TextureFilterOptions::TFO_ANISOTROPIC);
      return;
    default:tex->setTextureFiltering(Ogre::TextureFilterOptions::TFO_TRILINEAR);
      return;
  }
}

void setTransform(const nif::compound::TexDesc::NiTextureTransform &transform,
                  Ogre::TextureUnitState *tex) {
  const Ogre::Matrix4 translation = [&transform]() {
    Ogre::Matrix4 t{Ogre::Matrix4::IDENTITY};
    t.makeTrans(transform.translation.u, transform.translation.v, 0.0f);
    return t;
  }();

  const Ogre::Matrix4 scale = [&transform]() {
    Ogre::Matrix4 s{Ogre::Matrix4::IDENTITY};
    s.setScale(Ogre::Vector3{transform.scale.u, transform.scale.v, 1.0f});
    return s;
  }();

  // TODO: Is transform.rotation really an anti-clockwise rotation in radians?
  const Ogre::Matrix4 rotation = [&transform]() {
    using namespace Ogre;
    Matrix4 r{Ogre::Matrix4::IDENTITY};
    r.makeTransform(Vector3::ZERO, Vector3::UNIT_SCALE,
                    Quaternion{Radian{transform.rotation}, Vector3::UNIT_Z});
    return r;
  }();

  const Ogre::Matrix4 center = [&transform]() {
    Ogre::Matrix4 c{Ogre::Matrix4::IDENTITY};
    c.makeTrans(transform.center.u, transform.center.v, 0.0f);
    return c;
  }();

  const Ogre::Matrix4 centerInv{center.inverse()};

  switch (transform.transformMethod) {
    case nif::Enum::TransformMethod::MayaDeprecated: {
      const auto trans{center * rotation * centerInv * translation * scale};
      tex->setTextureTransform(trans);
      break;
    }
    case nif::Enum::TransformMethod::Max: {
      const auto trans{center * scale * rotation * translation * centerInv};
      tex->setTextureTransform(trans);
      break;
    }
    case nif::Enum::TransformMethod::Maya: {
      Ogre::Matrix4 fromMaya{};
      fromMaya.makeTransform(Ogre::Vector3::UNIT_Y,
                             Ogre::Vector3::NEGATIVE_UNIT_Y,
                             Ogre::Quaternion::ZERO);
      const auto trans
          {center * rotation * centerInv * fromMaya * translation * scale};
      tex->setTextureTransform(trans);
      break;
    }
    default:tex->setTextureTransform(Ogre::Matrix4::IDENTITY);
      break;
  }
}

BoundedSubmesh
MeshLoaderState::parseNiTriBasedGeom(const nif::NiTriBasedGeom &block,
                                     LoadStatus &tag,
                                     const Ogre::Matrix4 &transform) {
  // NiTriBasedGeom blocks determine discrete pieces of geometry with a single
  // material and texture, and so translate to Ogre::SubMesh objects.
  // If a submesh with this name already exists, then it is either already
  // loaded or in the process of loading.
  Tagger tagger{tag};
  if (tag == LoadStatus::Loaded) {
    auto submesh{mMesh->getSubMesh(block.name.str())};
    if (submesh) {
      // TODO: How to get the bounding box once the submesh has been created?
      return {submesh, {}};
    } else {
      throw std::runtime_error(
          "NiTriBasedGeom marked as loaded but submesh does not exist");
    }
  }
  auto submesh{mMesh->createSubMesh(block.name.str())};

  submesh->useSharedVertices = false;

  std::vector<nif::compound::Vector3> bitangents{};
  std::vector<nif::compound::Vector3> tangents{};

  // For normal mapping we need tangent and bitangent information given inside
  // an NiBinaryExtraData block. For low versions, the extra data is arranged
  // like a linked list, and for high versions it's an array.
  // TODO: Support the linked list version
  if (block.extraDataArray) {
    for (const auto &extraDataRef : *block.extraDataArray) {
      if (!checkRefType<nif::NiBinaryExtraData>(extraDataRef)) continue;
      auto &binaryExtraData{getBlock<nif::NiBinaryExtraData>(extraDataRef)};

      if (binaryExtraData.name) {
        const std::string name{binaryExtraData.name->str()};
        if (name.find("Tangent space") == 0) {
          // Format seems to be t1 t2 t3 ... b1 b2 b3 ...
          const std::size_t bytesPerList{binaryExtraData.data.dataSize / 2};
          const std::size_t bytesPerVert{3u * sizeof(float)};
          const std::size_t vertsPerList{bytesPerList / bytesPerVert};
          bitangents.resize(vertsPerList);
          tangents.resize(vertsPerList);

          // Poor naming choices, maybe?
          const nif::basic::Byte *bytes{binaryExtraData.data.data.data()};
          std::memcpy(tangents.data(), bytes, bytesPerList);
          std::memcpy(bitangents.data(), bytes + bytesPerList, bytesPerList);
        }
      }
    }
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
      std::find_if(block.properties.begin(), block.properties.end(),
                   [this](const auto &property) {
                     return checkRefType<nif::NiMaterialProperty>(property);
                   });

  // Find the texturing property, if any
  auto taggedTexIt =
      std::find_if(block.properties.begin(), block.properties.end(),
                   [this](const auto &property) {
                     return checkRefType<nif::NiTexturingProperty>(property);
                   });

  // It is ok to have a material but no texture
  if (taggedMatIt != block.properties.end()) {
    auto &taggedMat{mBlocks[static_cast<int32_t>(*taggedMatIt)]};
    auto &matBlock{dynamic_cast<nif::NiMaterialProperty &>(*taggedMat.block)};
    auto &matTag{taggedMat.tag};

    // If the material has already been loaded, then by the assumption that each
    // material has a unique texture (if any), the associated texture must
    // already have been loaded and attached to the material. We can load the
    // material (presumably from a cache) and skip the texture loading.
    // Without this assumption we would have to clone the material and attach
    // the new texture.
    if (matTag == LoadStatus::Loaded) {
      const auto material{parseNiMaterialProperty(matBlock, matTag)};
      submesh->setMaterialName(material->getName(), material->getGroup());
    } else {
      const auto material{parseNiMaterialProperty(matBlock, matTag)};
      submesh->setMaterialName(material->getName(), material->getGroup());
      auto pass{material->getTechnique(0)->getPass(0)};

      // The texture may or may not have already been loaded, but it has
      // definitely not been attached to the material.
      if (taggedTexIt != block.properties.end()) {
        auto &taggedTex{mBlocks[static_cast<int32_t>(*taggedTexIt)]};
        auto texBlock
            {dynamic_cast<nif::NiTexturingProperty &>(*taggedTex.block)};
        auto &texTag{taggedTex.tag};
        auto family{parseNiTexturingProperty(texBlock, texTag, pass)};
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
  const auto totalTrans{transform * getTransform(block)};

  // TODO: Replace with getBlock when tags aren't necessary
  const auto dataRef{static_cast<int32_t>(block.data)};
  if (dataRef < 0 || dataRef >= mBlocks.vertex_set().size()) {
    throw std::out_of_range("Nonexistent reference");
  }
  auto &taggedDataBlock{mBlocks[dataRef]};
  auto &dataBlock{*taggedDataBlock.block};

  auto &geometryData
      {dynamic_cast<nif::NiGeometryData &>(*taggedDataBlock.block)};

  if (taggedDataBlock.tag == LoadStatus::Loaded) {
    auto bbox{getBoundingBox(geometryData, totalTrans)};
    return {submesh, bbox};
  }

  std::unique_ptr<Ogre::IndexData> indexData{};
  Tagger dataTagger{taggedDataBlock.tag};

  // TODO: Replace this with a virtual function somehow?
  if (dynamic_cast<nif::NiTriShapeData *>(&dataBlock)) {
    auto &triShapeData{dynamic_cast<nif::NiTriShapeData &>(dataBlock)};
    indexData = generateIndexData(triShapeData);
    submesh->operationType = Ogre::RenderOperation::OT_TRIANGLE_LIST;
  } else if (dynamic_cast<nif::NiTriStripsData *>(&dataBlock)) {
    auto &triStripsData{dynamic_cast<nif::NiTriStripsData &>(dataBlock)};
    indexData = generateIndexData(triStripsData);
    submesh->operationType = Ogre::RenderOperation::OT_TRIANGLE_STRIP;
  }

  auto vertexData{generateVertexData(geometryData, totalTrans,
                                     &bitangents, &tangents)};

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
MeshLoaderState::parseNiMaterialProperty(const nif::NiMaterialProperty &block,
                                         LoadStatus &tag) {
  auto &materialManager{Ogre::MaterialManager::getSingleton()};

  Tagger tagger{tag};
  // Materials should be nif local, so a reasonable strategy would be name the
  // Ogre::Material by the mesh name followed by the nif material name.
  // Unfortunately, nif material names are not necessarily unique, even within
  // a nif file. We therefore resort to using the block index.
  // TODO: This is way more work than we need to do here
  // TODO: Get rid of this, it's hideous.
  auto comp = [this, &block](auto i) {
    return dynamic_cast<const nif::NiMaterialProperty *>(&*mBlocks[i].block)
        == &block;
  };
  auto it{std::find_if(mBlocks.vertex_set().begin(),
                       mBlocks.vertex_set().end(),
                       comp)};

  std::string meshName{mMesh->getName()};
  const std::string materialName
      {meshName.append("/").append(std::to_string(*it))};
  if (tag == LoadStatus::Loaded) {
    auto material{materialManager.getByName(materialName, mMesh->getGroup())};
    if (material) {
      return std::move(material);
    } else {
      throw std::runtime_error(
          "NiMaterialProperty marked as loaded but material does not exist");
    }
  }

  auto material{materialManager.create(materialName, mMesh->getGroup())};

  auto technique{material->getTechnique(0)};
  auto pass{technique->getPass(0)};

  pass->setAmbient(conversions::fromNif(block.ambientColor));

  auto diffuse{conversions::fromNif(block.diffuseColor)};
  diffuse.a = block.alpha;
  pass->setDiffuse(diffuse);

  auto specular{conversions::fromNif(block.specularColor)};
  specular.a = block.alpha;
  pass->setSpecular(specular);

  pass->setEmissive(conversions::fromNif(block.emissiveColor));

  // TODO: How to convert from nif glossiness to Ogre shininess?
  pass->setShininess(block.glossiness);

  using AutoConst = Ogre::GpuProgramParameters::AutoConstantType;
  pass->setVertexProgram("genericMaterial_vs_glsl", true);
  auto vsParams{pass->getVertexProgramParameters()};
  vsParams->setNamedAutoConstant("world",
                                 AutoConst::ACT_WORLD_MATRIX);
  vsParams->setNamedAutoConstant("worldInverseTranspose",
                                 AutoConst::ACT_INVERSE_TRANSPOSE_WORLD_MATRIX);
  vsParams->setNamedAutoConstant("worldViewProj",
                                 AutoConst::ACT_WORLDVIEWPROJ_MATRIX);
  vsParams->setNamedAutoConstant("viewPos",
                                 AutoConst::ACT_CAMERA_POSITION);

  pass->setFragmentProgram("genericMaterial_fs_glsl", true);
  auto fsParams{pass->getFragmentProgramParameters()};
  const int numLights{8};
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

  return std::move(material);
}

std::unique_ptr<Ogre::TextureUnitState>
MeshLoaderState::parseTexDesc(const nif::compound::TexDesc *tex,
                              Ogre::Pass *parent,
                              const std::optional<std::string> &textureOverride) {
  auto textureUnit{std::make_unique<Ogre::TextureUnitState>(parent)};

  setClampMode(tex->clampMode, textureUnit.get());
  setFilterMode(tex->filterMode, textureUnit.get());

  textureUnit->setTextureCoordSet(tex->uvSet);

  if (tex->hasTextureTransform && *tex->hasTextureTransform) {
    setTransform(*tex->textureTransform, textureUnit.get());
  }

  const auto &source{getBlock<nif::NiSourceTexture &>(tex->source)};
  setSourceTexture(source, textureUnit.get(), textureOverride);

  return textureUnit;
}

MeshLoaderState::MeshLoaderState(Ogre::Mesh *mesh, BlockGraph untaggedBlocks)
    : mMesh(mesh) {
  boost::copy_graph(untaggedBlocks, mBlocks);

  std::vector<boost::default_color_type> colorMap(boost::num_vertices(mBlocks));
  auto propertyMap = boost::make_iterator_property_map(
      colorMap.begin(), boost::get(boost::vertex_index, mBlocks));

  boost::depth_first_search(mBlocks, TBGVisitor(*this), propertyMap);
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
  auto &niObject = *taggedNiObject.block;
  auto &tag = taggedNiObject.tag;

  if (dynamic_cast<const nif::NiTriBasedGeom *>(&niObject)) {
    const auto &geom{dynamic_cast<const nif::NiTriBasedGeom &>(niObject)};
    auto[submesh, subBbox] = state.parseNiTriBasedGeom(geom, tag, transform);
    auto bbox{state.mMesh->getBounds()};
    bbox.merge(subBbox);
    state.mMesh->_setBounds(bbox);
  } else if (dynamic_cast<const nif::NiNode *>(&niObject)) {
    auto &niNode{dynamic_cast<const nif::NiNode &>(niObject)};
    transform = transform * getTransform(niNode);
  }
}

// If we have finished reading an NiNode, then we can remove its transformation.
void TBGVisitor::finish_vertex(vertex_descriptor v, const Graph &g) {
  auto &niObject{*g[v].block};
  if (dynamic_cast<const nif::NiNode *>(&niObject)) {
    auto &niNode{dynamic_cast<const nif::NiNode &>(niObject)};
    transform = transform * getTransform(niNode).inverse();
  }
}

} // namespace engine::nifloader