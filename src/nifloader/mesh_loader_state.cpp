#include "fs/path.hpp"
#include "math/conversions.hpp"
#include "nifloader/logging.hpp"
#include "nifloader/mesh_loader_state.hpp"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/graph/copy.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <OgreHardwareBufferManager.h>
#include <OgreMaterialManager.h>
#include <OgrePass.h>
#include <OgreSkeletonManager.h>
#include <OgreTechnique.h>
#include <OgreTextureManager.h>
#include <algorithm>
#include <numeric>

namespace oo {

Ogre::AxisAlignedBox
getBoundingBox(const nif::NiGeometryData &block, Ogre::Matrix4 transformation) {
  const auto fltMin = std::numeric_limits<float>::lowest();
  const auto fltMax = std::numeric_limits<float>::max();
  Ogre::Vector3 bboxMin{fltMax, fltMax, fltMax};
  Ogre::Vector3 bboxMax{fltMin, fltMin, fltMin};

  if (!block.hasVertices || block.vertices.empty()) return {};

  for (const auto &vertex : block.vertices) {
    const auto v = transformation * oo::fromBSCoordinates(vertex);
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
  const auto expected = (v2 - v1).crossProduct(v3 - v1);
  const auto actual = (n1 + n2 + n3) / 3.0f;
  // Coordinate system is right-handed so this is positive for an
  // anticlockwise winding order and negative for a clockwise order.
  return expected.dotProduct(actual) > 0.0f;
}

long numCCWTriangles(const nif::NiTriShapeData &block) {
  assert(block.hasNormals);
  const auto &tris{block.triangles};
  return std::count_if(tris.begin(), tris.end(), [&](const auto &tri) {
    return oo::isWindingOrderCCW(
        qvm::convert_to<Ogre::Vector3>(block.vertices[tri.v1]),
        qvm::convert_to<Ogre::Vector3>(block.normals[tri.v1]),
        qvm::convert_to<Ogre::Vector3>(block.vertices[tri.v2]),
        qvm::convert_to<Ogre::Vector3>(block.normals[tri.v2]),
        qvm::convert_to<Ogre::Vector3>(block.vertices[tri.v3]),
        qvm::convert_to<Ogre::Vector3>(block.normals[tri.v3]));
  });
}

std::unique_ptr<Ogre::VertexData>
generateVertexData(const nif::NiGeometryData &block,
                   Ogre::Matrix4 transformation,
                   std::vector<nif::compound::Vector3> *bitangents,
                   std::vector<nif::compound::Vector3> *tangents,
                   std::vector<BoneBinding> *boneBindings) {
  // Ogre expects a heap allocated raw pointer, but to improve exception safety
  // we construct an unique_ptr then relinquish control of it to Ogre.
  auto vertexData{std::make_unique<Ogre::VertexData>()};
  vertexData->vertexCount = block.numVertices;
  auto vertDecl{vertexData->vertexDeclaration};
  auto vertBind{vertexData->vertexBufferBinding};
  auto *hwBufMgr{Ogre::HardwareBufferManager::getSingletonPtr()};

  // Specify the order of data in the vertex buffer. This is per vertex,
  // so the vertices, normals etc will have to be interleaved in the buffer.
  std::ptrdiff_t offset{0};
  std::size_t vertSize{0};
  const unsigned short source{0};

  // Vertices
  vertDecl->addElement(source, vertSize, Ogre::VET_FLOAT3, Ogre::VES_POSITION);
  vertSize += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
  offset += 3;

  if (boneBindings) {
    // Blend indices
    vertDecl->addElement(source, vertSize, Ogre::VET_FLOAT4,
                         Ogre::VES_BLEND_INDICES);
    vertSize += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT4);
    offset += 4;

    // Blend weights
    vertDecl->addElement(source, vertSize, Ogre::VET_FLOAT4,
                         Ogre::VES_BLEND_WEIGHTS);
    vertSize += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT4);
    offset += 4;
  }

  // Normals
  vertDecl->addElement(source, vertSize, Ogre::VET_FLOAT3, Ogre::VES_NORMAL);
  vertSize += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
  offset += 3;

  // Vertex colours
  vertDecl->addElement(source, vertSize, Ogre::VET_FLOAT3, Ogre::VES_DIFFUSE);
  vertSize += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
  offset += 3;

  // UVs
  vertDecl->addElement(source, vertSize, Ogre::VET_FLOAT2,
                       Ogre::VES_TEXTURE_COORDINATES, 0);
  vertSize += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT2);
  offset += 2;

  // Bitangents
  vertDecl->addElement(source, vertSize, Ogre::VET_FLOAT3, Ogre::VES_BINORMAL);
  vertSize += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
  offset += 3;

  // Tangents
  vertDecl->addElement(source, vertSize, Ogre::VET_FLOAT3, Ogre::VES_TANGENT);
  vertSize += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
  offset += 3;

  // Normal vectors are not translated and transform with the inverse
  // transpose of the transformation matrix. We will also need this for tangents
  // and bitangents so we compute it now.
  const Ogre::Matrix4 normalTransformation = [&]() {
    auto trans{transformation};
    trans.setTrans(Ogre::Vector3::ZERO);
    return trans.inverse().transpose();
  }();

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
      const auto v{transformation * oo::fromBSCoordinates(vertex)};
      *it = v.x;
      *(it + 1) = v.y;
      *(it + 2) = v.z;
      // Necessary so on the last iteration of the loop we don't overshoot the
      // end of the buffer. Doing so is technically UB, because even though we
      // don't explicitly dereference an out-of-bounds iterator, += acts as if
      // we are calling ++ a bunch of times, and ++ has a precondition that the
      // iterator is dereferencable. Clang and GCC don't notice (or they do and
      // just don't care since they're doing pointer arithmetic) but MSVC cares.
      it += std::min(offset, (vertexBuffer.end() - it));
    }
    localOffset += 3;
  } else {
    throw std::runtime_error("NiGeometryData has no vertices");
  }

  if (boneBindings) {
    // Blend indices
    {
      auto it = vertexBuffer.begin() + localOffset;
      for (const auto &binding : *boneBindings) {
        *it = static_cast<float>(binding.indices[0]);
        *(it + 1) = static_cast<float>(binding.indices[1]);
        *(it + 2) = static_cast<float>(binding.indices[2]);
        *(it + 3) = static_cast<float>(binding.indices[3]);
        it += std::min(offset, (vertexBuffer.end() - it));
      }
      localOffset += 4;
    }

    // Blend weights
    {
      auto it = vertexBuffer.begin() + localOffset;
      for (const auto &binding : *boneBindings) {
        *it = binding.weights[0];
        *(it + 1) = binding.weights[1];
        *(it + 2) = binding.weights[2];
        *(it + 3) = binding.weights[3];
        it += std::min(offset, (vertexBuffer.end() - it));
      }
      localOffset += 4;
    }
  }

  // Normals
  if (block.hasNormals) {
    auto it = vertexBuffer.begin() + localOffset;
    for (const auto &normal : block.normals) {
      const auto n{normalTransformation * oo::fromBSCoordinates(normal)};
      *it = n.x;
      *(it + 1) = n.y;
      *(it + 2) = n.z;
      it += std::min(offset, (vertexBuffer.end() - it));
    }
    localOffset += 3;
  } else {
    // If a mesh doesn't have any normals then default to up vectors.
    // This lets distant terrain meshes work, which have their normals provided
    // by a normal map perturbing an implicit vertical normal.
    auto it = vertexBuffer.begin() + localOffset;
    for (int i = 0; i < block.numVertices; ++i) {
      *it = 0.0f;
      *(it + 1) = 1.0f;
      *(it + 2) = 0.0f;
      it += std::min(offset, (vertexBuffer.end() - it));
    }
    localOffset += 3;
    oo::nifloaderLogger()->warn("NiGeometryData has no normals");
  }

  // Vertex colours
  if (block.hasVertexColors) {
    auto it = vertexBuffer.begin() + localOffset;
    for (const auto &col : block.vertexColors) {
      *it = col.r;
      *(it + 1) = col.g;
      *(it + 2) = col.b;
      it += std::min(offset, (vertexBuffer.end() - it));
    }
    localOffset += 3;
  } else {
    // If a mesh doesn't have any vertex colours then we default to white
    auto it = vertexBuffer.begin() + localOffset;
    for (int i = 0; i < block.numVertices; ++i) {
      std::fill(it, it + 3, 1.0f);
      it += std::min(offset, (vertexBuffer.end() - it));
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
      it += std::min(offset, (vertexBuffer.end() - it));
    }
    localOffset += 2;
  }

  // Bitangents
  if (bitangents) {
    auto it = vertexBuffer.begin() + localOffset;
    for (const auto &bitangent : *bitangents) {
      const auto bt{normalTransformation * oo::fromBSCoordinates(bitangent)};
      *it = bt.x;
      *(it + 1) = bt.y;
      *(it + 2) = bt.z;
      it += std::min(offset, (vertexBuffer.end() - it));
    }
    localOffset += 3;
  } else {
    // TODO: Compute the bitangents if they don't exist
  }

  // Tangents
  if (tangents) {
    auto it = vertexBuffer.begin() + localOffset;
    for (const auto &tangent : *tangents) {
      const auto t{normalTransformation * oo::fromBSCoordinates(tangent)};
      *it = t.x;
      *(it + 1) = t.y;
      *(it + 2) = t.z;
      it += std::min(offset, (vertexBuffer.end() - it));
    }
    localOffset += 3;
  } else {
    // TODO: Compute the tangents if they don't exist
  }

  // Copy the vertex buffer into a hardware buffer, and link the buffer to
  // the vertex declaration.
  const auto usage{Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY};
  const auto bpv{vertSize};

  auto hwBuf{hwBufMgr->createVertexBuffer(bpv, block.numVertices, usage)};
  hwBuf->writeData(0, hwBuf->getSizeInBytes(), vertexBuffer.data(), true);

  vertBind->setBinding(source, hwBuf);

  return vertexData;
}

std::unique_ptr<Ogre::IndexData>
generateIndexData(const nif::NiTriShapeData &block) {
  auto &hwBufMgr{Ogre::HardwareBufferManager::getSingleton()};

  // We can assume that compound::Triangle has no padding and std::vector
  // is sequential, so can avoid copying the faces.
  auto indexBuffer{reinterpret_cast<const uint16_t *>(block.triangles.data())};

  const auto usage{Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY};
  const auto itype{Ogre::HardwareIndexBuffer::IT_16BIT};
  const std::size_t numIndices{3u * block.numTriangles};

  // Copy the triangle (index) buffer into a hardware buffer.
  auto hwBuf{hwBufMgr.createIndexBuffer(itype, numIndices, usage)};
  hwBuf->writeData(0, hwBuf->getSizeInBytes(), indexBuffer, true);

  auto indexData{std::make_unique<Ogre::IndexData>()};
  indexData->indexBuffer = hwBuf;
  indexData->indexCount = numIndices;
  indexData->indexStart = 0;
  return indexData;
}

std::unique_ptr<Ogre::IndexData>
generateIndexData(const nif::NiTriStripsData &block) {
  auto &hwBufMgr = Ogre::HardwareBufferManager::getSingleton();

  const auto usage{Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY};
  const auto itype{Ogre::HardwareIndexBuffer::IT_16BIT};
  const std::size_t numIndices{std::accumulate(block.stripLengths.begin(),
                                               block.stripLengths.end(), 0u)};

  std::vector<uint16_t> buf{};
  buf.reserve(numIndices);
  for (const auto &strip : block.points) {
    buf.insert(buf.end(), strip.begin(), strip.end());
  }

  auto hwBuf{hwBufMgr.createIndexBuffer(itype, numIndices, usage)};
  hwBuf->writeData(0, hwBuf->getSizeInBytes(), buf.data(), true);

  auto indexData{std::make_unique<Ogre::IndexData>()};
  indexData->indexBuffer = hwBuf;
  indexData->indexCount = numIndices;
  indexData->indexStart = 0;
  return indexData;
}

std::unique_ptr<Ogre::IndexData>
generateIndexData(const nif::NiGeometryData &block, oo::SubMesh *submesh) {
  if (dynamic_cast<const nif::NiTriShapeData *>(&block)) {
    submesh->operationType = Ogre::RenderOperation::OT_TRIANGLE_LIST;
    const auto &triShape{dynamic_cast<const nif::NiTriShapeData &>(block)};
    return oo::generateIndexData(triShape);
  } else if (dynamic_cast<const nif::NiTriStripsData *>(&block)) {
    submesh->operationType = Ogre::RenderOperation::OT_TRIANGLE_STRIP;
    const auto &triStrips{dynamic_cast<const nif::NiTriStripsData &>(block)};
    return oo::generateIndexData(triStrips);
  }
  return std::unique_ptr<Ogre::IndexData>{};
}

std::vector<BoneBinding> getBoneBindings(const nif::NiSkinPartition &skin) {
  // Vertices on the border of two partitions are present in both, so this is
  // an overestimate that we will trim down by recording the largest index of
  // the written vertices.
  const auto &blocks{skin.skinPartitionBlocks};
  const std::size_t numVertices{std::accumulate(
      blocks.begin(), blocks.end(), 0u, [](std::size_t a, const auto &block) {
        return a + block.numVertices;
      })};
  std::size_t maxIndex{0};

  std::vector<BoneBinding> bindings(numVertices);

  for (const auto &partition : blocks) {
    if (partition.numWeightsPerVertex > 4) {
      // TODO: Warn about having too many weights per vertex
    }
    if (partition.hasVertexMap.has_value() && !*partition.hasVertexMap) {
      throw std::runtime_error("NiSkinPartition has no vertex map");
    }
    if (partition.hasVertexWeights.has_value()
        && !*partition.hasVertexWeights) {
      throw std::runtime_error("NiSkinPartition has no vertex weights");
    }
    if (!partition.hasBoneIndices) {
      throw std::runtime_error("NiSkinPartition has no bone indices");
    }

    for (std::size_t i{0}; i < partition.numVertices; ++i) {
      // Get the actual index of this vertex in the mesh
      std::size_t j = partition.vertexMap[i];
      if (j > maxIndex) maxIndex = j;
      auto len{std::min<std::size_t>(partition.numWeightsPerVertex, 4u)};

      // Copy the bone indices
      {
        auto begin{partition.boneIndices[i].begin()};
        std::transform(begin, begin + len, bindings[j].indices.begin(),
                       [&partition](auto idx) {
                         return partition.bones[static_cast<std::size_t>(idx)];
                       });
      }

      // Copy the weights
      {
        auto begin{partition.vertexWeights[i].begin()};
        std::copy(begin, begin + len, bindings[j].weights.begin());
      }
    }
  }

  // Trim vertices that are actually just overlap between partitions
  bindings.resize(maxIndex + 1u);

  return bindings;
}

BoneAssignments
getBoneAssignments(const oo::BlockGraph &g, const nif::NiTriBasedGeom &block) {
  if (auto instRef{block.skinInstance}; instRef && *instRef) {
    const auto &instance{oo::getBlock<nif::NiSkinInstance>(g, *instRef)};
    const auto &bones{instance.bones};
    if (auto partRef{instance.skinPartition}; partRef && *partRef) {
      const auto &partition{oo::getBlock<nif::NiSkinPartition>(g, *partRef)};
      BoneAssignments boneAssignments;
      boneAssignments.bindings = oo::getBoneBindings(partition);

      // Get the bone names of the blend indices.
      for (auto &binding : boneAssignments.bindings) {
        for (auto idx : binding.indices) {
          if (idx >= boneAssignments.names.size()) {
            boneAssignments.names.resize(idx + 1u);
          } else if (boneAssignments.names[idx].empty()) {
            const auto &node{oo::getBlock<nif::NiNode>(g, bones[idx])};
            boneAssignments.names[idx] = node.name.str();
          }
        }
      }

      return boneAssignments;
    }
  }

  return {};
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
      tex->setTextureName(oo::Path{texFile.filename.string.str()}.c_str());
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
  using AddressingMode = Ogre::TextureAddressingMode;

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
    Ogre::Matrix4 r{Ogre::Matrix4::IDENTITY};
    r.makeTransform(Ogre::Vector3::ZERO, Ogre::Vector3::UNIT_SCALE,
                    Ogre::Quaternion{Ogre::Radian{transform.rotation},
                                     Ogre::Vector3::UNIT_Z});
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

void
setMaterialProperties(const nif::NiMaterialProperty &block, Ogre::Pass *pass) {
  pass->setAmbient(oo::fromNif(block.ambientColor));

  auto diffuse{oo::fromNif(block.diffuseColor)};
  diffuse.a = block.alpha;
  pass->setDiffuse(diffuse);

  auto specular{oo::fromNif(block.specularColor)};
  specular.a = block.alpha;
  pass->setSpecular(specular);

  pass->setEmissive(oo::fromNif(block.emissiveColor));

  // TODO: How to convert from nif glossiness to Ogre shininess?
  pass->setShininess(block.glossiness);
}

void addStaticVertexShader(Ogre::Pass *pass) {
  using AutoConst = Ogre::GpuProgramParameters::AutoConstantType;
  pass->setVertexProgram("genericMaterial_vs_glsl", true);
  auto vsParams{pass->getVertexProgramParameters()};
  vsParams->setNamedAutoConstant("world",
                                 AutoConst::ACT_WORLD_MATRIX);
  vsParams->setNamedAutoConstant("worldInverseTranspose",
                                 AutoConst::ACT_INVERSE_TRANSPOSE_WORLD_MATRIX);
  vsParams->setNamedAutoConstant("worldViewProj",
                                 AutoConst::ACT_WORLDVIEWPROJ_MATRIX);
}

void addSkinnedVertexShader(Ogre::Pass *pass) {
  using AutoConst = Ogre::GpuProgramParameters::AutoConstantType;
  pass->setVertexProgram("genericSkinnedMaterial_vs_glsl", true);
  auto vsParams{pass->getVertexProgramParameters()};
  vsParams->setNamedAutoConstant("worldInverseTranspose",
                                 AutoConst::ACT_INVERSE_TRANSPOSE_WORLD_MATRIX);
  vsParams->setNamedAutoConstant("viewProj",
                                 AutoConst::ACT_VIEWPROJ_MATRIX);
  vsParams->setNamedAutoConstant("worldMatrixArray",
                                 AutoConst::ACT_WORLD_MATRIX_ARRAY_3x4);
}

void addDeferredFragmentShader(Ogre::Pass *pass) {
  using AutoConst = Ogre::GpuProgramParameters::AutoConstantType;
  pass->setFragmentProgram("genericMaterial_fs_glsl", true);
  auto fsParams{pass->getFragmentProgramParameters()};
//  const int numLights{8};
  fsParams->setNamedConstant("diffuseMap", 0);
  fsParams->setNamedConstant("normalMap", 1);
  fsParams->setNamedAutoConstant("matShininess",
                                 AutoConst::ACT_SURFACE_SHININESS);
  fsParams->setNamedAutoConstant("matDiffuse",
                                 AutoConst::ACT_SURFACE_DIFFUSE_COLOUR);
}

void addForwardFragmentShader(Ogre::Pass *pass) {
  using AutoConst = Ogre::GpuProgramParameters::AutoConstantType;
  pass->setFragmentProgram("genericMaterial_forward_fs_glsl", true);
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
  fsParams->setNamedAutoConstant("viewPos",
                                 AutoConst::ACT_CAMERA_POSITION);
  fsParams->setNamedAutoConstant("ambientLightColor",
                                 AutoConst::ACT_AMBIENT_LIGHT_COLOUR);
  fsParams->setNamedAutoConstant("matShininess",
                                 AutoConst::ACT_SURFACE_SHININESS);
  fsParams->setNamedAutoConstant("matDiffuse",
                                 AutoConst::ACT_SURFACE_DIFFUSE_COLOUR);
  fsParams->setNamedAutoConstant("matSpecular",
                                 AutoConst::ACT_SURFACE_SPECULAR_COLOUR);
}

TangentData getTangentData(const nif::NiBinaryExtraData &extraData) {
  TangentData out{};
  if (!extraData.name) return out;
  //C++20: if (!extraData.name->str()->starts_with("Tangent space")) return out;
  if (!boost::starts_with(extraData.name->str(), "Tangent space")) return out;

  // Format seems to be t1 t2 t3 ... b1 b2 b3 ...
  const std::size_t bytesPerList{extraData.data.dataSize / 2};
  const std::size_t bytesPerVert{3u * sizeof(float)};
  const std::size_t vertsPerList{bytesPerList / bytesPerVert};
  out.bitangents.resize(vertsPerList);
  out.tangents.resize(vertsPerList);

  // Poor naming choices, maybe?
  const nif::basic::Byte *bytes{extraData.data.data.data()};
  std::memcpy(out.tangents.data(), bytes, bytesPerList);
  std::memcpy(out.bitangents.data(), bytes + bytesPerList, bytesPerList);

  return out;
}

TangentData parseTangentData(const oo::BlockGraph &g,
                             const nif::NiExtraDataArray &dataArray) {
  const auto pred = [&g](auto ref) {
    const auto &data{oo::getBlock<nif::NiExtraData>(g, ref)};
    //C++20: return data.name && data.name->str()->starts_with("Tangent space");
    return data.name && boost::starts_with(data.name->str(), "Tangent space");
  };
  const auto begin{dataArray.begin()}, end{dataArray.end()};
  const auto it{std::find_if(begin, end, pred)};
  if (it != end && oo::checkRefType<nif::NiBinaryExtraData>(g, *it)) {
    return oo::getTangentData(oo::getBlock<nif::NiBinaryExtraData>(g, *it));
  }
  return TangentData{};
}

std::unique_ptr<Ogre::TextureUnitState>
parseTexDesc(const oo::BlockGraph &g,
             const nif::compound::TexDesc *tex,
             Ogre::Pass *parent,
             const std::optional<std::string> &textureOverride) {
  auto textureUnit{std::make_unique<Ogre::TextureUnitState>(parent)};

  oo::setClampMode(tex->clampMode, textureUnit.get());
  oo::setFilterMode(tex->filterMode, textureUnit.get());

  textureUnit->setTextureCoordSet(tex->uvSet);

  if (tex->hasTextureTransform && *tex->hasTextureTransform) {
    oo::setTransform(*tex->textureTransform, textureUnit.get());
  }

  const auto &source{oo::getBlock<nif::NiSourceTexture>(g, tex->source)};
  oo::setSourceTexture(source, textureUnit.get(), textureOverride);

  return textureUnit;
}

TextureFamily parseNiTexturingProperty(const oo::BlockGraph &g,
                                       const nif::NiTexturingProperty &block,
                                       Ogre::Pass *pass) {
  // Nif ApplyMode is for vertex colors, which are currently unsupported
  // TODO: Support ApplyMode with vertex colors

  TextureFamily family{};

  if (block.hasBaseTexture) {
    family.base = oo::parseTexDesc(g, &block.baseTexture, pass);
    // Normal mapping is automatically turned on if a normal map exists. If one
    // doesn't exist, then we use a flat normal map.
    auto &texMgr{Ogre::TextureManager::getSingleton()};
    if (auto normalMap = oo::makeNormalPath(family.base->getTextureName());
        texMgr.resourceExists(normalMap, pass->getResourceGroup())) {
      family.normal = oo::parseTexDesc(g, &block.baseTexture, pass, normalMap);
    } else {
      family.normal = oo::parseTexDesc(g, &block.baseTexture, pass,
                                       "textures/flat_n.dds");
    }
  }

  if (block.hasDarkTexture) {
    family.dark = oo::parseTexDesc(g, &block.darkTexture, pass);
  }

  if (block.hasDetailTexture) {
    family.detail = oo::parseTexDesc(g, &block.detailTexture, pass);
  }

  if (block.hasGlossTexture) {
    family.gloss = oo::parseTexDesc(g, &block.glossTexture, pass);
  }

  if (block.hasGlowTexture) {
    family.glow = oo::parseTexDesc(g, &block.glowTexture, pass);
  }

  if (!block.hasDecal0Texture) return family;
  family.decals.emplace_back(oo::parseTexDesc(g, &block.decal0Texture, pass));

  if (!block.hasDecal1Texture) return family;
  family.decals.emplace_back(oo::parseTexDesc(g, &block.decal1Texture, pass));

  if (!block.hasDecal2Texture) return family;
  family.decals.emplace_back(oo::parseTexDesc(g, &block.decal2Texture, pass));

  if (!block.hasDecal3Texture) return family;
  family.decals.emplace_back(oo::parseTexDesc(g, &block.decal3Texture, pass));

  return family;
}

bool attachTextureProperty(const oo::BlockGraph &g,
                           const nif::NiPropertyArray &properties,
                           Ogre::Pass *pass) {
  auto it{std::find_if(properties.begin(), properties.end(), [&g](auto ref) {
    return oo::checkRefType<nif::NiTexturingProperty>(g, ref);
  })};
  if (it == properties.end()) return false;

  const auto &texBlock{oo::getBlock<nif::NiTexturingProperty>(g, *it)};

  auto family{oo::parseNiTexturingProperty(g, texBlock, pass)};

  if (!family.base) return true;
  family.base->setName("diffuse");
  pass->addTextureUnitState(family.base.release());

  if (!family.normal) return true;
  family.normal->setName("normal");
  pass->addTextureUnitState(family.normal.release());

  // TODO: Support more than base textures
  return true;
}

bool attachStencilProperty(const oo::BlockGraph &g,
                           const nif::NiPropertyArray &properties,
                           Ogre::Pass *pass) {
  auto it{std::find_if(properties.begin(), properties.end(), [&](auto ref) {
    return oo::checkRefType<nif::NiStencilProperty>(g, ref);
  })};
  if (it == properties.end()) return false;

  const auto &stencilBlock{oo::getBlock<nif::NiStencilProperty>(g, *it)};

  auto parseCullMode = [](nif::Enum::StencilDrawMode mode) {
    switch (mode) {
      case nif::Enum::StencilDrawMode::DRAW_CCW: return Ogre::CULL_CLOCKWISE;
      case nif::Enum::StencilDrawMode::DRAW_CW: return Ogre::CULL_ANTICLOCKWISE;
      default: [[fallthrough]];
      case nif::Enum::StencilDrawMode::DRAW_CCW_OR_BOTH: [[fallthrough]];
      case nif::Enum::StencilDrawMode::DRAW_BOTH: return Ogre::CULL_NONE;
    }
  };
  pass->setCullingMode(parseCullMode(stencilBlock.drawMode));

  // Stenciling needs the cooperation of the compositor, and is unsupported.
  if (stencilBlock.stencilEnabled) {
    oo::nifloaderLogger()->warn("NiStencilProperty uses stencil, this is "
                                "unsupported");
  }

  return true;
}

bool attachAlphaProperty(const oo::BlockGraph &g,
                         const nif::NiPropertyArray &properties,
                         Ogre::Pass *pass) {
  auto it{std::find_if(properties.begin(), properties.end(), [&](auto ref) {
    return oo::checkRefType<nif::NiAlphaProperty>(g, ref);
  })};
  if (it == properties.end()) return false;

  const auto &alphaBlock{oo::getBlock<nif::NiAlphaProperty>(g, *it)};

  // OGRE's defaults are ok in this case but we'll be explicit.
  if (!alphaBlock.alphaBlendingEnabled) {
    pass->setSceneBlendingOperation(Ogre::SceneBlendOperation::SBO_ADD);
    pass->setSceneBlending(Ogre::SceneBlendFactor::SBF_ONE,
                           Ogre::SceneBlendFactor::SBF_ZERO);
    return true;
  }

  using NifMode = nif::NiAlphaProperty::BlendMode;
  using OgreMode = Ogre::SceneBlendFactor;
  auto parseBlendMode = [](NifMode mode) -> OgreMode {
    switch (mode) {
      case NifMode::GL_ONE: return OgreMode::SBF_ONE;
      case NifMode::GL_ZERO: return OgreMode::SBF_ZERO;
      case NifMode::GL_SRC_COLOR: return OgreMode::SBF_SOURCE_COLOUR;
      case NifMode::GL_ONE_MINUS_SRC_COLOR: return OgreMode::SBF_ONE_MINUS_SOURCE_COLOUR;
      case NifMode::GL_DST_COLOR: return OgreMode::SBF_DEST_COLOUR;
      case NifMode::GL_ONE_MINUS_DST_COLOR: return OgreMode::SBF_ONE_MINUS_DEST_COLOUR;
      case NifMode::GL_SRC_ALPHA: return OgreMode::SBF_SOURCE_ALPHA;
      case NifMode::GL_ONE_MINUS_SRC_ALPHA: return OgreMode::SBF_ONE_MINUS_SOURCE_ALPHA;
      case NifMode::GL_DST_ALPHA: return OgreMode::SBF_DEST_ALPHA;
      case NifMode::GL_ONE_MINUS_DST_ALPHA: return OgreMode::SBF_ONE_MINUS_DEST_ALPHA;
        // Presumably source alpha with a saturate/clamp, unsupported.
      case NifMode::GL_SRC_ALPHA_SATURATE:[[fallthrough]];
      default: return OgreMode::SBF_SOURCE_ALPHA;
    }
  };

  const auto srcFactor{parseBlendMode(alphaBlock.sourceBlendMode)};
  const auto dstFactor{parseBlendMode(alphaBlock.destinationBlendMode)};

  pass->setSceneBlendingOperation(Ogre::SceneBlendOperation::SBO_ADD);
  pass->setSceneBlending(srcFactor, dstFactor);

  using NifComp = nif::NiAlphaProperty::TestMode;
  using OgreComp = Ogre::CompareFunction;
  auto parseAlphaComp = [](NifComp comp) -> OgreComp {
    switch (comp) {
      case NifComp::GL_ALWAYS: return OgreComp::CMPF_ALWAYS_PASS;
      case NifComp::GL_LESS: return OgreComp::CMPF_LESS;
      case NifComp::GL_EQUAL: return OgreComp::CMPF_EQUAL;
      case NifComp::GL_LEQUAL: return OgreComp::CMPF_LESS_EQUAL;
      case NifComp::GL_GREATER: return OgreComp::CMPF_GREATER;
      case NifComp::GL_NOTEQUAL: return OgreComp::CMPF_NOT_EQUAL;
      case NifComp::GL_GEQUAL: return OgreComp::CMPF_GREATER_EQUAL;
      case NifComp::GL_NEVER: return OgreComp::CMPF_ALWAYS_FAIL;
      default: return OgreComp::CMPF_ALWAYS_PASS;
    }
  };

  if (alphaBlock.alphaTestEnabled) {
    pass->setAlphaRejectFunction(parseAlphaComp(alphaBlock.alphaTestMode));
    pass->setAlphaRejectValue(alphaBlock.threshold);
  }

  pass->setTransparentSortingEnabled(!alphaBlock.disableTriangleSorting);
  pass->setDepthWriteEnabled(true);

  return true;
}

std::shared_ptr<Ogre::Material>
parseNiMaterialProperty(const oo::BlockGraph &g,
                        const std::string &meshName,
                        const std::string &meshGroup,
                        const nif::NiMaterialProperty &block) {
  // Materials should be nif local, so a reasonable strategy would be name the
  // Ogre::Material by the mesh name followed by the nif material name.
  // Unfortunately, nif material names are not necessarily unique, even within
  // a nif file. We therefore resort to using the block index.
  const auto it{oo::getBlockIndex<nif::NiMaterialProperty>(g, block)};
  const auto materialName{(meshName + "/").append(std::to_string(*it))};

  auto &matMgr{Ogre::MaterialManager::getSingleton()};
  if (auto material{matMgr.getByName(materialName, meshGroup)}) {
    return std::move(material);
  }

  auto material{matMgr.create(materialName, meshGroup)};
  auto pass{material->getTechnique(0)->getPass(0)};
  // Sometimes the original material name is needed in another part of the
  // engine. Store the relevant ones in the pass.
  if (boost::iequals(block.name.str(), "skin")) {
    pass->setName("skin");
  }

  oo::setMaterialProperties(block, pass);

  return std::move(material);
}

bool attachMaterialProperty(const oo::BlockGraph &g,
                            const nif::NiPropertyArray &properties,
                            oo::SubMesh *submesh) {
  auto it{std::find_if(properties.begin(), properties.end(), [&g](auto ref) {
    return oo::checkRefType<nif::NiMaterialProperty>(g, ref);
  })};
  if (it == properties.end()) return false;

  const auto &matBlock{oo::getBlock<nif::NiMaterialProperty>(g, *it)};

  const std::string &name{submesh->parent->getName()};
  const std::string &group{submesh->parent->getGroup()};
  const auto material{oo::parseNiMaterialProperty(g, name, group, matBlock)};
  submesh->setMaterialName(material->getName(), material->getGroup());

  return true;
}

BoundedSubmesh parseNiTriBasedGeom(const oo::BlockGraph &g,
                                   oo::Mesh *mesh,
                                   const nif::NiTriBasedGeom &block,
                                   const Ogre::Matrix4 &transform) {
  // If this submesh has already been loaded, return it.
  // Can return an empty bounding box because if the submesh has already been
  // loaded then its bbox has already been merged in; we don't need it again.
  {
    const auto &nameMap{mesh->getSubMeshNameMap()};
    if (auto it{nameMap.find(block.name.str())}; it != nameMap.end()) {
      return {mesh->getSubMeshes()[it->second].get(), {}};
    }
  }

  auto submesh{mesh->createSubMesh(block.name.str())};

  auto boneAssignments{oo::getBoneAssignments(g, block)};
  const auto hasBones{!boneAssignments.bindings.empty()};

  if (oo::attachMaterialProperty(g, block.properties, submesh)) {
    auto &matMgr{Ogre::MaterialManager::getSingleton()};
    auto matPtr{matMgr.getByName(submesh->getMaterialName(),
                                 submesh->getMaterialGroup())};
    auto *pass{matPtr->getTechnique(0)->getPass(0)};

    hasBones ? oo::addSkinnedVertexShader(pass)
             : oo::addStaticVertexShader(pass);

    oo::attachTextureProperty(g, block.properties, pass);
    if (oo::attachAlphaProperty(g, block.properties, pass)) {
      oo::addForwardFragmentShader(pass);
      pass->getParent()->setSchemeName("NoGBuffer");
    } else {
      oo::addDeferredFragmentShader(pass);
    }
    oo::attachStencilProperty(g, block.properties, pass);
  }

  const auto &geomData{oo::getBlock<nif::NiGeometryData>(g, block.data)};

  // For normal mapping we need tangent and bitangent information given inside
  // an NiBinaryExtraData block. For low versions, the extra data is arranged
  // like a linked list, and for high versions it's an array.
  auto[bitangents, tangents] = [&g, &block]() {
    // TODO: Support the linked list version
    return block.extraDataArray ? oo::parseTangentData(g, *block.extraDataArray)
                                : TangentData{};
  }();

  // Ogre::SubMeshes cannot have transformations applied to them (that is
  // reserved for Ogre::SceneNodes), so we will apply it to all the vertex
  // information manually.
  const auto totalTrans{transform * getTransform(block)};
  auto vertexData{oo::generateVertexData(geomData, totalTrans,
                                         &bitangents, &tangents,
                                         hasBones ? &boneAssignments.bindings
                                                  : nullptr)};
  auto indexData{oo::generateIndexData(geomData, submesh)};

  // Transfer ownership to Ogre
  submesh->vertexData = std::move(vertexData);
  submesh->indexData = std::move(indexData);
  if (hasBones) submesh->boneNames = std::move(boneAssignments.names);

  return {submesh, getBoundingBox(geomData, totalTrans)};
}

MeshLoaderState::MeshLoaderState(oo::Mesh *mesh, Graph blocks)
    : mMesh(mesh), mBlocks(std::move(blocks)) {
  std::vector<boost::default_color_type> colorMap(boost::num_vertices(mBlocks));
  const auto propertyMap{boost::make_iterator_property_map(
      colorMap.begin(), boost::get(boost::vertex_index, mBlocks))};

  boost::depth_first_search(mBlocks, *this, propertyMap);
}

// This is a new connected component so we need to reset the transformation to
// the identity. NB: This vertex will still be discovered so setting the
// transformation to the vertex's will result in it being applied twice.
void MeshLoaderState::start_vertex(vertex_descriptor, const Graph &) {
  mTransform = Ogre::Matrix4::IDENTITY;
}

// If this vertex corresponds to a geometry block, then load it with the current
// transformation. If it's an NiNode, update the current transformation so that
// this child transformation occurs before any parent ones.
void MeshLoaderState::discover_vertex(vertex_descriptor v, const Graph &g) {
  auto &niObject = *g[v];

  if (dynamic_cast<const nif::NiTriBasedGeom *>(&niObject)) {
    const auto &geom{dynamic_cast<const nif::NiTriBasedGeom &>(niObject)};
    auto[submesh, subBbox] = oo::parseNiTriBasedGeom(mBlocks, mMesh, geom,
                                                     mTransform);
    auto bbox{mMesh->getBounds()};
    bbox.merge(subBbox);
    mMesh->_setBounds(bbox);
    mMesh->_setBoundingSphereRadius(Ogre::Math::boundingRadiusFromAABB(bbox));
  } else if (dynamic_cast<const nif::NiNode *>(&niObject)) {
    auto &niNode{dynamic_cast<const nif::NiNode &>(niObject)};
    mTransform = mTransform * getTransform(niNode);
  }
}

// If we have finished reading an NiNode, then we can remove its transformation.
void MeshLoaderState::finish_vertex(vertex_descriptor v, const Graph &g) {
  auto &niObject{*g[v]};
  if (dynamic_cast<const nif::NiNode *>(&niObject)) {
    auto &niNode{dynamic_cast<const nif::NiNode &>(niObject)};
    mTransform = mTransform * getTransform(niNode).inverse();
  }
}

void createMesh(oo::Mesh *mesh, oo::BlockGraph::vertex_descriptor start,
                const oo::BlockGraph &g) {
  const auto &rootBlock{*g[start]};
  if (!dynamic_cast<const nif::NiNode *>(&rootBlock)) {
    OGRE_EXCEPT(Ogre::Exception::ERR_INVALIDPARAMS,
                "Cannot create a Mesh with a root node that is not an NiNode",
                "oo::createMesh");
  }

  oo::nifloaderLogger()->info("createMesh({})", mesh->getName());

  // It is assumed that the transformation of the root node will be applied via
  // it's Ogre::Node transformation, and thus we do not need to bake it in.
  const Ogre::Matrix4 transform{Ogre::Matrix4::IDENTITY};

  for (const auto &e : boost::make_iterator_range(boost::out_edges(start, g))) {
    auto v{boost::target(e, g)};
    const auto &block{*g[v]};
    if (!dynamic_cast<const nif::NiTriBasedGeom *>(&block)) continue;

    const auto &geom{static_cast<const nif::NiTriBasedGeom &>(block)};
    auto[submesh, subBounds]{oo::parseNiTriBasedGeom(g, mesh, geom, transform)};
    auto bounds{mesh->getBounds()};
    bounds.merge(subBounds);
    mesh->_setBounds(bounds);
    mesh->_setBoundingSphereRadius(Ogre::Math::boundingRadiusFromAABB(bounds));
  }
}

void createRawMesh(oo::Mesh *mesh, const Ogre::MaterialPtr &matPtr,
                   oo::BlockGraph::vertex_descriptor start,
                   const oo::BlockGraph &g) {
  const auto &rootBlock{*g[start]};
  if (!dynamic_cast<const nif::NiTriBasedGeom *>(&rootBlock)) {
    OGRE_EXCEPT(Ogre::Exception::ERR_INVALIDPARAMS,
                "Cannot create a Mesh with a root node that is not an "
                "NiTriBasedGeom",
                "oo::createRawMesh");
  }

  oo::nifloaderLogger()->info("createRawMesh({})", mesh->getName());

  // There is no root node here, just a single NiTriBasedGeom submesh.
  const Ogre::Matrix4 transform{Ogre::Matrix4::IDENTITY};
  const auto &geom{static_cast<const nif::NiTriBasedGeom &>(rootBlock)};
  auto[submesh, bounds]{oo::parseNiTriBasedGeom(g, mesh, geom, transform)};
  submesh->setMaterialName(matPtr->getName(), matPtr->getGroup());
  mesh->_setBounds(bounds);
  mesh->_setBoundingSphereRadius(Ogre::Math::boundingRadiusFromAABB(bounds));
}

} // namespace oo