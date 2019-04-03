#include "entity.hpp"
#include "mesh_manager.hpp"
#include "settings.hpp"
#include "subentity.hpp"
#include <boost/range/adaptor/indexed.hpp>
#include <OgreOptimisedUtil.h>
#include <OgreRoot.h>
#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreSkeletonManager.h>
#include <OgreTagPoint.h>
#include <OgreSkeletonInstance.h>
#include <spdlog/spdlog.h>

namespace oo {

class SkeletonState {
 private:
  friend class Entity;

  /// Skeleton to use for animation.
  Ogre::SkeletonInstance mSkeleton;
  /// The states of all animations available for this entity, based on its
  /// current skeleton.
  Ogre::AnimationStateSet mAnimationStateSet{};
  /// Cached bone matrices of the skeleton.
  std::vector<Ogre::Affine3, oo::Affine3Allocator> mBoneMatrices{};
  /// The frame that animations were last updated.
  uint64_t mFrameLastUpdated{};

  /// Utility method to copy a SkeletonInstance. Have to go the long way round
  /// as SkeletonInstance doesn't have a copy constructor.
  static Ogre::SkeletonPtr getSkeletonByHandle(Ogre::ResourceHandle hnd) {
    auto &skelMgr{Ogre::SkeletonManager::getSingleton()};
    return std::static_pointer_cast<Ogre::Skeleton>(skelMgr.getByHandle(hnd));
  }

 public:
  explicit SkeletonState(const Ogre::SkeletonPtr &masterSkeleton)
      : mSkeleton(masterSkeleton) {
    mSkeleton.load();
    mBoneMatrices.resize(mSkeleton.getNumBones(), Ogre::Affine3::IDENTITY);
    mSkeleton._initAnimationState(&mAnimationStateSet);
  }

  ~SkeletonState() = default;
  SkeletonState(const SkeletonState &other)
      : mSkeleton(getSkeletonByHandle(other.mSkeleton.getHandle())),
        mAnimationStateSet(other.mAnimationStateSet),
        mBoneMatrices(other.mBoneMatrices),
        mFrameLastUpdated(other.mFrameLastUpdated) {
    mSkeleton.load();
    mSkeleton._initAnimationState(&mAnimationStateSet);
  }

  SkeletonState &operator=(const SkeletonState &) = delete;
  SkeletonState(SkeletonState &&) = delete;
  SkeletonState &operator=(SkeletonState &&) = delete;

  uint16_t getNumBones() const {
    return mSkeleton.getNumBones();
  }

  Ogre::Bone *getBone(const std::string &name) const {
    return mSkeleton.getBone(name);
  }

  bool isAnimationDirty() const {
    return mSkeleton.getManualBonesDirty() ||
        mFrameLastUpdated != mAnimationStateSet.getDirtyFrameNumber();
  }

  void updateBoneMatrices() {
    mSkeleton.setAnimationState(mAnimationStateSet);
    mSkeleton._getBoneMatrices(mBoneMatrices.data());
    mFrameLastUpdated = mAnimationStateSet.getDirtyFrameNumber();
  }
};

Entity::Entity(const std::string &name, oo::MeshPtr mesh)
    : MovableObject(name), mMesh(std::move(mesh)) {
  _initialise();
}

Entity::~Entity() {
  _deinitialise();
}

void Entity::buildSubEntityList(const oo::MeshPtr &mesh,
                                oo::Entity::SubEntityList &list) {
  const auto &subMeshes{mesh->getSubMeshes()};
  list.reserve(subMeshes.size());

  for (const auto &subMesh : subMeshes) {
    // Can't use make_unique as constructor is private.
    auto &subEntity{list.emplace_back(
        absl::WrapUnique(new oo::SubEntity(this, subMesh.get())))};
    if (subMesh->isMatInitialised()) {
      subEntity->setMaterialName(subMesh->getMaterialName(),
                                 subMesh->getMaterialGroup());
    }
  }
}

const oo::MeshPtr &Entity::getMesh() const {
  return mMesh;
}

const Entity::SubEntityList &Entity::getSubEntities() const {
  return mSubEntityList;
}

oo::Entity *Entity::clone(const std::string &name) {
  if (!mManager) {
    OGRE_EXCEPT(Ogre::Exception::ERR_ITEM_NOT_FOUND,
                "Cannot clone an Entity that wasn't created through a "
                "SceneManager",
                "Entity::clone");
  }

  Ogre::NameValuePairList params{
      {"mesh", mMesh->getName()},
      {"resourceGroup", mMesh->getGroup()}
  };
  auto *entity{static_cast<oo::Entity *>(mManager->createMovableObject(
      name, oo::EntityFactory::FACTORY_TYPE_NAME, &params))};

  // Could be cloning a deinitialised entity.
  if (!mIsInitialised) {
    // Since the constructor of entity was called, it' probably initialised so
    // we should deinitialise it to be consistent with ourselves.
    entity->_deinitialise();
    return entity;
  }

  for (const auto &elem : mSubEntityList | boost::adaptors::indexed()) {
    const auto &matPtr{elem.value()->getMaterial()};
    entity->getSubEntities()[elem.index()]->setMaterial(matPtr);
  }

  // Make a copy of our skeleton state, do not share it with the original.
  if (mSkeletonState) {
    entity->mSkeletonState =
        std::make_shared<oo::SkeletonState>(*mSkeletonState);
    entity->mFrameLastUpdated = mFrameLastUpdated;
  }

  return entity;
}

bool Entity::isInitialised() const {
  return mIsInitialised;
}

void Entity::_initialise(bool forceReinitialise) {
  if (forceReinitialise) _deinitialise();
  else if (mIsInitialised) return;

  mMesh->load();
  if (!mMesh->isLoaded()) return;

  buildSubEntityList(mMesh, mSubEntityList);

  // Note mParentNode might be a tagpoint, so mParentNode != getParentSceneNode
  // in general.
  if (mParentNode) getParentSceneNode()->needUpdate();

  mMeshStateCount = mMesh->getStateCount();
  mIsInitialised = true;
}

void Entity::_deinitialise() {
  if (!mIsInitialised) return;

  // clear + shrink_to_fit mimics (almost) OGRE which explicitly frees memory.

  mSubEntityList.clear();
  mSubEntityList.shrink_to_fit();
  detachAllObjectsImpl();

  if (mSkeletonState) {
    mBoneWorldMatrices.clear();
    mBoneWorldMatrices.shrink_to_fit();
    mSkeletonState.reset();
  }

  mIsInitialised = false;
}

//===----------------------------------------------------------------------===//
// MovableObject overrides
//===----------------------------------------------------------------------===//

void Entity::_notifyCurrentCamera(Ogre::Camera *camera) {
  Ogre::MovableObject::_notifyCurrentCamera(camera);

  if (mParentNode) {
    for (auto &subEntity : mSubEntityList) subEntity->_invalidateCameraCache();
  }

  for (auto &[k, v] : mChildObjectList) v->_notifyCurrentCamera(camera);
}

const Ogre::AxisAlignedBox &Entity::getBoundingBox() const {
  if (!mMesh->isLoaded()) {
    mFullAABB.setNull();
    return mFullAABB;
  }

  mFullAABB = mMesh->getBounds();
  for (const auto &[_, child] : mChildObjectList) {
    // Need a local transform so can't use child->_getParentNodeFullTransform().
    auto *tagPoint{static_cast<Ogre::TagPoint *>(child->getParentNode())};
    auto aabb{child->getBoundingBox()};
    aabb.transform(tagPoint->_getFullLocalTransform());
    mFullAABB.merge(aabb);
  }

  return mFullAABB;
}

const Ogre::AxisAlignedBox &Entity::getWorldBoundingBox(bool derive) const {
  if (derive) {
    for (auto &[_, child] : mChildObjectList) {
      (void) child->getWorldBoundingBox(true);
    }
  }

  return Ogre::MovableObject::getWorldBoundingBox(derive);
}

const Ogre::Sphere &Entity::getWorldBoundingSphere(bool derive) const {
  if (derive) {
    for (auto &[_, child]: mChildObjectList) {
      (void) child->getWorldBoundingSphere(true);
    }
  }

  return Ogre::MovableObject::getWorldBoundingSphere(derive);
}

float Entity::getBoundingRadius() const {
  return mMesh->getBoundingSphereRadius();
}

void Entity::_updateRenderQueue(Ogre::RenderQueue *queue) {
  if (!mIsInitialised) return;
  if (mMeshStateCount != mMesh->getStateCount()) _initialise(true);

  // This is subtly different to Ogre but I think easier to read; if rend has a
  // custom group but not priority, and the movable object has a custom
  // priority, then that priority will be used instead of the default. If the
  // user cares what their priority is, they should set it explicitly.

  auto getQueue = [this](Ogre::RenderQueue *queue, oo::SubEntity *rend) {
    if (rend->isRenderQueueGroupSet()) return rend->getRenderQueueGroup();
    else if (mRenderQueueIDSet) return mRenderQueueID;
    else return queue->getDefaultQueueGroup();
  };

  auto getPriority = [this](Ogre::RenderQueue *queue, oo::SubEntity *rend) {
    if (rend->isRenderQueuePrioritySet()) return rend->getRenderQueuePriority();
    else if (mRenderQueuePrioritySet) return mRenderQueuePriority;
    else return queue->getDefaultRenderablePriority();
  };

  for (const auto &subEntity : mSubEntityList) {
    if (!subEntity->isVisible()) continue;
    auto *rend{subEntity.get()};
    queue->addRenderable(rend, getQueue(queue, rend), getPriority(queue, rend));
  }

  if (hasSkeleton()) {
    updateAnimation();

    for (const auto &[_, child] : mChildObjectList) {
      if (child->isVisible()) child->_updateRenderQueue(queue);
    }
  }
}

const std::string &Entity::getMovableType() const {
  const static std::string typeName{EntityFactory::FACTORY_TYPE_NAME};
  return typeName;
}

uint32_t Entity::getTypeFlags() const {
  return Ogre::SceneManager::ENTITY_TYPE_MASK;
}

void Entity::visitRenderables(Ogre::Renderable::Visitor *visitor, bool) {
  for (auto &subEntity : mSubEntityList) {
    visitor->visit(subEntity.get(), 0, false);
  }
}

//===----------------------------------------------------------------------===//
// Animation function definitions
//===----------------------------------------------------------------------===//

void Entity::updateAnimation() {
  if (!mIsInitialised) return;
  bool isAnimationDirty{mSkeletonState && mSkeletonState->isAnimationDirty()};

  if (isAnimationDirty) {
    mSkeletonState->updateBoneMatrices();
    if (!mChildObjectList.empty()) mParentNode->needUpdate();
  }

  // If this entity shares its skeleton then it another entity might have
  // updated the animation so isAnimationDirty is false, but this entity still
  // needs to be updated itself.
  bool isBonesDirty{isAnimationDirty || (mSkeletonState &&
      mFrameLastUpdated != mSkeletonState->mFrameLastUpdated)};

  const auto &fullTrans{_getParentNodeFullTransform()};
  if (mSkeletonState && (isBonesDirty || mLastParentXform != fullTrans)) {
    mLastParentXform = fullTrans;

    for (auto &[_, child] : mChildObjectList) {
      child->getParentNode()->_update(true, true);
    }

    auto *optUtil{Ogre::OptimisedUtil::getImplementation()};
    optUtil->concatenateAffineMatrices(mLastParentXform,
                                       mSkeletonState->mBoneMatrices.data(),
                                       mBoneWorldMatrices.data(),
                                       mSkeletonState->mBoneMatrices.size());
    mFrameLastUpdated = mSkeletonState->mFrameLastUpdated;
  }
}

Ogre::AnimationState *Entity::getAnimationState(const std::string &name) const {
  if (!mSkeletonState) {
    OGRE_EXCEPT(Ogre::Exception::ERR_ITEM_NOT_FOUND, "Entity is not animated.",
                "Entity::getAnimationState");
  }
  return mSkeletonState->mAnimationStateSet.getAnimationState(name);
}

bool Entity::hasAnimationState(const std::string &name) const {
  return mSkeletonState
      && mSkeletonState->mAnimationStateSet.hasAnimationState(name);
}

Ogre::AnimationStateSet *Entity::getAllAnimationStates() const {
  return mSkeletonState ? &mSkeletonState->mAnimationStateSet
                        : nullptr;
}

void Entity::refreshAvailableAnimationState() {
  if (mSkeletonState) {
    mSkeletonState->mSkeleton._refreshAnimationState(
        &mSkeletonState->mAnimationStateSet);
  }
}

void Entity::_updateAnimation() {
  if (mSkeletonState) updateAnimation();
}

bool Entity::_isAnimated() const {
  return mSkeletonState && (mSkeletonState->mSkeleton.hasManualBones()
      || mSkeletonState->mAnimationStateSet.hasEnabledAnimationState());
}

bool Entity::_isSkeletonAnimated() const {
  // Only skeletal animation is supported.
  return _isAnimated();
}

bool Entity::hasSkeleton() const {
  return mSkeletonState != nullptr;
}

Ogre::SkeletonInstance *Entity::getSkeleton() const {
  return &mSkeletonState->mSkeleton;
}

void Entity::setSkeleton(const Ogre::SkeletonPtr &skeletonPtr) {
  mSkeletonState = std::make_shared<oo::SkeletonState>(skeletonPtr);
  setSkeletonImpl();
}

void Entity::shareSkeleton(oo::Entity *other) {
  mSkeletonState = other->mSkeletonState;
  setSkeletonImpl();
}

void Entity::setSkeletonImpl() {
  mBoneWorldMatrices.resize(mSkeletonState->getNumBones(), mLastParentXform);
  // If getBone() fails to find a bone it will throw, but will never return
  // nullptr. Trying to attach a skeleton that is not compatible with the mesh
  // is not an error, but does not attach the skeleton.
  try {
    for (const auto &subEntity : mSubEntityList) {
      const auto &bones{subEntity->getSubMesh()->boneNames};
      auto &indexMap{subEntity->mBlendIndexToBoneIndexMap};
      indexMap.clear();

      std::transform(bones.begin(), bones.end(), std::back_inserter(indexMap),
                     [&](const std::string &name) {
                       return mSkeletonState->getBone(name)->getHandle();
                     });
    }
  } catch (const Ogre::Exception &e) {
    spdlog::get(oo::LOG)->error(e.getFullDescription());
    mSkeletonState.reset();
    mBoneWorldMatrices.clear();
  }
}

//===----------------------------------------------------------------------===//
// MovableObject attachment functions
//===----------------------------------------------------------------------===//

Ogre::TagPoint *
Entity::attachObjectToBone(const std::string &boneName,
                           Ogre::MovableObject *movable,
                           const Ogre::Quaternion &offsetOrientation,
                           const Ogre::Vector3 &offsetPosition) {
  // C++20: if (mChildObjectList.contains(movable->getName())) {
  if (mChildObjectList.find(movable->getName()) != mChildObjectList.end()) {
    OGRE_EXCEPT(Ogre::Exception::ERR_DUPLICATE_ITEM,
                "An object with the name " + movable->getName()
                    + " is already attached.",
                "Entity::attachObjectToBone");
  }

  if (movable->isAttached()) {
    OGRE_EXCEPT(Ogre::Exception::ERR_INVALIDPARAMS,
                "Object is already attached to a SceneNode or Bone",
                "Entity::attachObjectToBone");
  }

  if (!mSkeletonState) {
    OGRE_EXCEPT(Ogre::Exception::ERR_INVALIDPARAMS,
                "Entity has no Skeleton to attach an object to.",
                "Entity::attachObjectToBone");
  }

  auto *bone{mSkeletonState->getBone(boneName)};
  if (!bone) {
    OGRE_EXCEPT(Ogre::Exception::ERR_INVALIDPARAMS,
                "Cannot locate Bone named " + boneName + ".",
                "Entity::attachObjectToBone");
  }

  auto *tagPoint{mSkeletonState->mSkeleton.createTagPointOnBone(
      bone, offsetOrientation, offsetPosition)};
  tagPoint->setParentEntity(this);
  tagPoint->setChildObject(movable);

  attachObjectImpl(movable, tagPoint);

  if (mParentNode) mParentNode->needUpdate();

  return tagPoint;
}

void Entity::detachObjectFromBone(Ogre::MovableObject *movable) {
  for (const auto &[k, v] : mChildObjectList) {
    if (v != movable) continue;
    detachObjectImpl(v);
    mChildObjectList.erase(k);
    if (mParentNode) mParentNode->needUpdate();
    break;
  }
}

Ogre::MovableObject *
Entity::detachObjectFromBone(const std::string &name) {
  if (auto it{mChildObjectList.find(name)}; it != mChildObjectList.end()) {
    detachObjectImpl(it->second);
    mChildObjectList.erase(it);
    if (mParentNode) mParentNode->needUpdate();
    return it->second;
  }
  OGRE_EXCEPT(Ogre::Exception::ERR_ITEM_NOT_FOUND,
              "No child object called " + name + ".",
              "Entity::detachObjectFromBone");
}

void Entity::detachAllObjectsFromBones() {
  detachAllObjectsImpl();
  if (mParentNode) mParentNode->needUpdate();
}

Entity::ChildObjectListIterator Entity::getAttachedObjectIterator() {
  return {mChildObjectList.begin(), mChildObjectList.end()};
}

void Entity::attachObjectImpl(Ogre::MovableObject *movable,
                              Ogre::TagPoint *tagPoint) {
  mChildObjectList[movable->getName()] = movable;
  movable->_notifyAttached(tagPoint, true);
}

void Entity::detachObjectImpl(Ogre::MovableObject *movable) {
  auto *tagPoint{static_cast<Ogre::TagPoint *>(movable->getParentNode())};
  mSkeletonState->mSkeleton.freeTagPoint(tagPoint);
  movable->_notifyAttached(nullptr, true);
}

void Entity::detachAllObjectsImpl() {
  for (const auto &[k, v] : mChildObjectList) detachObjectImpl(v);
  mChildObjectList.clear();
}

//===----------------------------------------------------------------------===//
// EntityFactory definitions
//===----------------------------------------------------------------------===//
void EntityFactory::destroyInstance(gsl::owner<Ogre::MovableObject *> obj) {
  OGRE_DELETE obj;
}

const std::string &EntityFactory::getType() const {
  const static std::string typeName{FACTORY_TYPE_NAME};
  return typeName;
}

gsl::owner<Ogre::MovableObject *>
EntityFactory::createInstanceImpl(const std::string &name,
                                  const Ogre::NameValuePairList *params) {
  MeshPtr ptr{};
  std::string group{Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME};

  if (params) {
    if (auto it{params->find("resourceGroup")}; it != params->end()) {
      group = it->second;
    }

    if (auto it{params->find("mesh")}; it != params->end()) {
      auto &meshMgr{oo::MeshManager::getSingleton()};
      auto retrieveResult{meshMgr.createOrRetrieve(it->second, group)};
      ptr = std::dynamic_pointer_cast<oo::Mesh>(retrieveResult.first);
      if (ptr) ptr->load();
    }
  }

  if (!ptr) {
    OGRE_EXCEPT(Ogre::Exception::ERR_INVALIDPARAMS,
                "'mesh' parameter required when constructing a Entity.",
                "EntityFactory::createInstance");
  }

  return OGRE_NEW oo::Entity(name, ptr);
}

} // namespace oo