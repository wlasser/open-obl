#include "mesh/entity.hpp"
#include "mesh/subentity.hpp"
#include "mesh/submesh.hpp"
#include <OgreMaterialManager.h>
#include <OgreSceneNode.h>

namespace oo {

SubEntity::SubEntity(oo::Entity *parent, oo::SubMesh *subMesh) noexcept
    : mParent(parent),
      mSubMesh(subMesh),
      mIsVisible(true),
      mUseCustomRenderQueueId(false),
      mUseCustomRenderQueuePriority(false) {}

const std::string &SubEntity::getMaterialName() const {
  return mMaterialPtr ? mMaterialPtr->getName() : Ogre::BLANKSTRING;
}

void SubEntity::setMaterialName(const std::string &name,
                                const std::string &group) {
  // Don't bother with default materials because the defaults use the RTSS.
  mMaterialPtr = Ogre::MaterialManager::getSingleton().getByName(name, group);
  if (!mMaterialPtr) {
    OGRE_EXCEPT(Ogre::Exception::ERR_ITEM_NOT_FOUND,
                "Material " + name + " does not exist in group " + group + ".",
                "SubEntity::setMaterialName");
  }
  mMaterialPtr->load();
}

void SubEntity::setMaterial(const Ogre::MaterialPtr &material) {
  mMaterialPtr = material;
  if (!mMaterialPtr) {
    OGRE_EXCEPT(Ogre::Exception::ERR_ITEM_NOT_FOUND,
                "Cannot assign null material to SubEntity.",
                "SubEntity::setMaterial");
  }
  mMaterialPtr->load();
}

void SubEntity::setVisible(bool visible) {
  mIsVisible = visible;
}

bool SubEntity::isVisible() const {
  return mIsVisible;
}

void SubEntity::setRenderQueueGroup(uint8_t queueId) {
  mRenderQueueId = queueId;
  mUseCustomRenderQueueId = true;
}

void SubEntity::setRenderQueueGroupAndPriority(uint8_t queueId,
                                               uint16_t priority) {
  mRenderQueueId = queueId;
  mRenderQueuePriority = priority;
  mUseCustomRenderQueueId = true;
  mUseCustomRenderQueuePriority = true;
}

uint8_t SubEntity::getRenderQueueGroup() const {
  return mRenderQueueId;
}

uint16_t SubEntity::getRenderQueuePriority() const {
  return mRenderQueuePriority;
}

bool SubEntity::isRenderQueueGroupSet() const {
  return mUseCustomRenderQueueId;
}

bool SubEntity::isRenderQueuePrioritySet() const {
  return mUseCustomRenderQueuePriority;
}

oo::SubMesh *SubEntity::getSubMesh() const {
  return mSubMesh;
}

oo::Entity *SubEntity::getParent() const {
  return mParent;
}

void SubEntity::_invalidateCameraCache() {
  mCachedCamera = nullptr;
}

//===----------------------------------------------------------------------===//
// Renderable overrides
//===----------------------------------------------------------------------===//

const Ogre::MaterialPtr &SubEntity::getMaterial() const {
  return mMaterialPtr;
}

Ogre::Technique *SubEntity::getTechnique() const {
  return mMaterialPtr->getBestTechnique(0, this);
}

void SubEntity::getRenderOperation(Ogre::RenderOperation &op) {
  mSubMesh->_getRenderOperation(op);
}

void SubEntity::getWorldTransforms(Ogre::Matrix4 *xform) const {
  if (!mParent->mSkeletonState) {
    *xform = mParent->_getParentNodeFullTransform();
  } else if (mParent->_isSkeletonAnimated()) {
    for (auto index : mBlendIndexToBoneIndexMap) {
      *xform++ = mParent->mBoneWorldMatrices[index];
    }
  } else {
    std::fill_n(xform, mBlendIndexToBoneIndexMap.size(),
                mParent->_getParentNodeFullTransform());
  }
}

uint16_t SubEntity::getNumWorldTransforms() const {
  if (mParent->mBoneWorldMatrices.empty()) return 1u;
  return static_cast<uint16_t>(mBlendIndexToBoneIndexMap.size());
}

float SubEntity::getSquaredViewDepth(const Ogre::Camera *camera) const {
  if (mCachedCamera == camera) return mCachedCameraDist;
  // TODO: Support extremity points like OGRE so that this is more accurate.
  const float dist{mParent->getParentNode()->getSquaredViewDepth(camera)};
  mCachedCamera = camera;
  mCachedCameraDist = dist;
  return dist;
}

const Ogre::LightList &SubEntity::getLights() const {
  return mParent->queryLights();
}

bool SubEntity::getCastsShadows() const {
  return mParent->getCastShadows();
}

} // namespace oo