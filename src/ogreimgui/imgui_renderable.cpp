#include "ogreimgui/imgui_renderable.hpp"
#include <OgreHardwareBufferManager.h>
#include <imgui/imgui.h>
#include <type_traits>

namespace Ogre {

namespace {

// See Arthur O'Dwyer https://quuxplusone.github.io/blog/2018/04/02/false-v/
// Used for a 'static_assert(false)' that isn't UB.
template<class ...>
inline constexpr bool false_v = false;

template<class T>
HardwareIndexBuffer::IndexType getHardwareBufferType() {
  if constexpr (std::is_same_v<T, unsigned short>) {
    return HardwareIndexBuffer::IT_16BIT;
  } else if constexpr (std::is_same_v<T, unsigned int>) {
    return HardwareIndexBuffer::IT_32BIT;
  } else {
    static_assert(false_v<T>,
                  "Index buffer must store 'unsigned short' or 'unsigned int'");
    return HardwareIndexBuffer::IT_16BIT; // Unreachable
  }
}

}

ImGuiRenderable::ImGuiRenderable() {
  setUseIdentityProjection(true);
  setUseIdentityView(true);

  mIndexData = std::make_unique<IndexData>();
  mIndexData->indexStart = 0;
  mIndexData->indexCount = 0;
  mRenderOperation.indexData = mIndexData.get();

  mVertexData = std::make_unique<VertexData>();
  mVertexData->vertexStart = 0;
  mVertexData->vertexCount = 0;
  mRenderOperation.vertexData = mVertexData.get();

  mRenderOperation.operationType = RenderOperation::OT_TRIANGLE_LIST;
  mRenderOperation.useIndexes = true;
  mRenderOperation.useGlobalInstancingVertexBufferIsAvailable = false;

  auto vertDecl = mVertexData->vertexDeclaration;

  vertDecl->addElement(0, mBytesPerVertex, VET_FLOAT2, VES_POSITION);
  mBytesPerVertex += VertexElement::getTypeSize(VET_FLOAT2);

  // TODO: Ogre might want the this last, can it be swapped?
  vertDecl->addElement(0, mBytesPerVertex, VET_FLOAT2, VES_TEXTURE_COORDINATES);
  mBytesPerVertex += VertexElement::getTypeSize(VET_FLOAT2);

  vertDecl->addElement(0, mBytesPerVertex, VET_COLOUR, VES_DIFFUSE);
  mBytesPerVertex += VertexElement::getTypeSize(VET_COLOUR);
}

void ImGuiRenderable::generateIndexData(const ImDrawIdx *idxBuf,
                                        std::size_t numIndices) {
  auto &hwBufMgr{HardwareBufferManager::getSingleton()};
  const auto usage{HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY};
  const auto itype{getHardwareBufferType<ImDrawIdx>()};

  // TODO: Optimize creation to size changes only
  auto hwIdxBuf{hwBufMgr.createIndexBuffer(itype, numIndices, usage)};
  hwIdxBuf->writeData(0, hwIdxBuf->getSizeInBytes(), idxBuf, true);

  mIndexData->indexCount = numIndices;
}

void ImGuiRenderable::generateVertexData(const ImDrawVert *vertBuf,
                                         std::size_t numVerts) {
  auto &hwBufMgr{HardwareBufferManager::getSingleton()};
  auto vertBind{mVertexData->vertexBufferBinding};
  const auto usage{HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY};

  // TODO: Optimize creation to size changes only
  auto hwVertBuf{hwBufMgr.createVertexBuffer(mBytesPerVertex, numVerts, usage)};
  hwVertBuf->writeData(0, hwVertBuf->getSizeInBytes(), vertBuf, true);
  vertBind->setBinding(0, hwVertBuf);

  mVertexData->vertexCount = numVerts;
}

} // namespace Ogre