#include "mesh/mesh_manager.hpp"
#include <OgrePlane.h>
#include <OgreHardwareBufferManager.h>

namespace Ogre {

template<> oo::MeshManager *Singleton<oo::MeshManager>::msSingleton = nullptr;

} // namespace Ogre

namespace oo {

MeshManager &MeshManager::getSingleton() {
  assert(msSingleton);
  return *msSingleton;
}

MeshManager *MeshManager::getSingletonPtr() {
  return msSingleton;
}

MeshManager::MeshManager() {
  // Load order just before Ogre::MeshManager.
  mLoadOrder = 349.0f;
  mResourceType = "oo::Mesh";
  Ogre::ResourceGroupManager::getSingleton()
      ._registerResourceManager(mResourceType, this);
}

MeshManager::~MeshManager() {
  Ogre::ResourceGroupManager::getSingleton()
      ._unregisterResourceManager(mResourceType);
}

MeshPtr MeshManager::create(const std::string &name,
                            const std::string &group,
                            bool isManual,
                            Ogre::ManualResourceLoader *loader,
                            const Ogre::NameValuePairList *createParams) {
  return std::static_pointer_cast<Mesh>(
      createResource(name, group, isManual, loader, createParams));
}

MeshPtr MeshManager::createManual(const std::string &name,
                                  const std::string &group,
                                  Ogre::ManualResourceLoader *loader) {
  return create(name, group, true, loader);
}

MeshPtr MeshManager::getByName(const std::string &name,
                               const std::string &group) {
  return std::static_pointer_cast<Mesh>(getResourceByName(name, group));
}

float MeshManager::getBoundsPaddingFactor() const {
  return mBoundsPaddingFactor;
}

void MeshManager::setBoundsPaddingFactor(float paddingFactor) {
  mBoundsPaddingFactor = paddingFactor;
}

Ogre::Resource *MeshManager::createImpl(const std::string &name,
                                        Ogre::ResourceHandle handle,
                                        const std::string &group,
                                        bool isManual,
                                        Ogre::ManualResourceLoader *loader,
                                        const Ogre::NameValuePairList *) {
  return OGRE_NEW oo::Mesh(this, name, handle, group, isManual, loader);
}

MeshPtr MeshManager::createPlane(const std::string &name,
                                 const std::string &group,
                                 const Ogre::Plane &plane,
                                 float width,
                                 float height,
                                 int xSegments,
                                 int ySegments,
                                 bool normals,
                                 uint16_t uvSets,
                                 float uTile,
                                 float vTile,
                                 const Ogre::Vector3 &upVector) {
  MeshPtr ptr{createManual(name, group, nullptr)};
  auto *sub{ptr->createSubMesh()};
  sub->operationType = Ogre::RenderOperation::OT_TRIANGLE_LIST;

  const auto numVertices
      {static_cast<std::size_t>((xSegments + 1) * (ySegments + 1))};
  auto vertexData{std::make_unique<Ogre::VertexData>()};
  vertexData->vertexCount = numVertices;

  auto *vertDecl{vertexData->vertexDeclaration};
  auto *vertBind{vertexData->vertexBufferBinding};
  auto *hwBufMgr{ptr->getHardwareBufferManager()};

  std::size_t offset{0u};
  std::size_t vertSize{0u};
  const uint16_t source{0};

  // Vertices
  vertDecl->addElement(source, vertSize, Ogre::VET_FLOAT3, Ogre::VES_POSITION);
  vertSize += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
  offset += 3u;

  // Normals
  if (normals) {
    vertDecl->addElement(source, vertSize, Ogre::VET_FLOAT3, Ogre::VES_NORMAL);
    vertSize += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
    offset += 3u;
  }

  // UVs
  for (uint16_t i = 0u; i < uvSets; ++i) {
    vertDecl->addElement(source, vertSize, Ogre::VET_FLOAT2,
                         Ogre::VES_TEXTURE_COORDINATES, i);
    vertSize += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT2);
    offset += 2u;
  }

  // Default plane has normal vector UNIT_Y, height 0, and up vector UNIT_Z.
  // Then (up x normal, normal, up) form a right-handed coordinate system and
  // in standard coordinates have the combined matrix IDENTITY. The rotation
  // matrix into the actual plane's orientation is then the same but with
  // the actual plane's up, normal, and up x normal vectors.
  //
  // Normal vectors are not translated and transform with the inverse transpose,
  // but we have no scaling here so that's just the rot matrix.
  const Ogre::Vector3 up{upVector.normalisedCopy()};
  const Ogre::Vector3 normal{plane.normal.normalisedCopy()};
  const Ogre::Vector3 right{up.crossProduct(normal)};

  Ogre::Matrix3 rot;
  rot.SetColumn(0, up);
  rot.SetColumn(1, normal);
  rot.SetColumn(2, right);

  Ogre::Affine3 trans(Ogre::Matrix4{rot});
  trans.setTrans(normal * -plane.d);

  std::vector<float> vertexBuffer(offset * numVertices);

  const float xDelta{width / xSegments};
  const float yDelta{height / ySegments};
  const float uDelta{uTile / xSegments};
  const float vDelta{vTile / ySegments};

  auto it{vertexBuffer.begin()};
  for (int x = 0; x < xSegments + 1; ++x) {
    for (int y = 0; y < ySegments + 1; ++y) {
      const auto v{trans * Ogre::Vector3{x * xDelta, 0.0f, y * yDelta}};
      *it++ = v.x;
      *it++ = v.y;
      *it++ = v.z;

      if (normals) {
        const auto n{rot * Ogre::Vector3{0.0f, 1.0f, 0.0f}};
        *it++ = n.x;
        *it++ = n.y;
        *it++ = n.z;
      }

      for (uint16_t i = 0u; i < uvSets; ++i) {
        *it++ = x * uDelta;
        *it++ = y * vDelta;
      }
    }
  }

  {
    const auto usage{Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY};
    auto hwBuf{hwBufMgr->createVertexBuffer(vertSize, numVertices, usage)};
    hwBuf->writeData(0u, hwBuf->getSizeInBytes(), vertexBuffer.data(), true);
    vertBind->setBinding(source, hwBuf);
  }

  auto indexData{std::make_unique<Ogre::IndexData>()};
  const std::size_t numIndices{6u * xSegments * ySegments};
  indexData->indexCount = numIndices;
  indexData->indexStart = 0u;

  //  0--2/3
  //  | / |
  // 1/4--5
  std::vector<uint16_t> indexBuffer(numIndices);
  auto jt{indexBuffer.begin()};
  for (int x = 0; x < xSegments; ++x) {
    for (int y = 0; y < ySegments; ++y) {
      const auto base{static_cast<uint16_t>(6u * (ySegments * x + y))};
      *jt++ = base;
      *jt++ = base + 1u;
      *jt++ = base + ySegments + 1u;

      *jt++ = base + ySegments + 1u;
      *jt++ = base + 1u;
      *jt++ = base + ySegments + 2u;
    }
  }

  {
    const auto usage{Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY};
    const auto type{Ogre::HardwareIndexBuffer::IT_16BIT};
    auto hwBuf{hwBufMgr->createIndexBuffer(type, numIndices, usage)};
    hwBuf->writeData(0u, hwBuf->getSizeInBytes(), indexBuffer.data(), true);
    indexData->indexBuffer = hwBuf;
  }

  sub->vertexData = std::move(vertexData);
  sub->indexData = std::move(indexData);

  return ptr;
}

} // namespace oo
