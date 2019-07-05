#include "mesh/mesh.hpp"
#include "mesh/submesh.hpp"

namespace oo {

void SubMesh::setMaterialName(const std::string &matName,
                              const std::string &groupName) {
  mMaterialName = matName;
  mGroupName = groupName;
  mMatInitialized = true;
}

const std::string &SubMesh::getMaterialName() const {
  return mMaterialName;
}

const std::string &SubMesh::getMaterialGroup() const {
  return mGroupName;
}

bool SubMesh::isMatInitialised() const {
  return mMatInitialized;
}

void SubMesh::_getRenderOperation(Ogre::RenderOperation &rend) {
  rend.indexData = indexData.get();
  rend.useIndexes = indexData && indexData->indexCount != 0;
  rend.operationType = operationType;
  rend.vertexData = vertexData.get();
}

SubMesh *SubMesh::clone(const std::string &newName,
                        oo::Mesh *parentMesh) const {
  if (!parentMesh) parentMesh = parent;

  auto *bufMgr{parentMesh->getHardwareBufferManager()};
  auto *subMesh{parentMesh->createSubMesh(newName)};
  subMesh->operationType = operationType;
  subMesh->boneNames = boneNames;
  subMesh->parent = parentMesh;
  subMesh->mMatInitialized = mMatInitialized;
  subMesh->mMaterialName = mMaterialName;
  subMesh->mGroupName = mGroupName;

  // Ogre::VertexData::clone gives us a raw pointer allocated with OGRE_NEW but
  // we can assume that OGRE_NEW is just new.
  subMesh->vertexData.reset(vertexData->clone(true, bufMgr));
  subMesh->indexData.reset(indexData->clone(true, bufMgr));

  return subMesh;
}

} // namespace oo