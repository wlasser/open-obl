#ifndef OPENOBLIVION_ENTITY_HPP
#define OPENOBLIVION_ENTITY_HPP

#include "mesh.hpp"
#include <OgreMovableObject.h>

namespace oo {

using Affine3Allocator = Ogre::AlignedAllocator<Ogre::Affine3,
                                                OGRE_SIMD_ALIGNMENT>;
class EntityFactory;
class SubEntity;
class SkeletonState;

class Entity : public Ogre::MovableObject {
 public:
  using ChildObjectList = std::map<std::string, Ogre::MovableObject *>;
  using ChildObjectListIterator = Ogre::MapIterator<ChildObjectList>;

 private:
  friend class oo::EntityFactory;
  friend class oo::SubEntity;

  using SubEntityList = std::vector<std::unique_ptr<oo::SubEntity>>;

  Entity(const std::string &name, oo::MeshPtr mesh);

  /// The mesh defining the geometry of this entity.
  oo::MeshPtr mMesh{};
  /// The constituent subentities of this entity. Each corresponds to a submesh
  /// of `mMesh`.
  SubEntityList mSubEntityList{};
  /// Child objects attached to bones of this entity's skeleton.
  ChildObjectList mChildObjectList{};
  /// Skeleton and animation state, possibly shared with other entities.
  std::shared_ptr<oo::SkeletonState> mSkeletonState{};

  uint64_t mFrameLastUpdated{};
  bool mIsInitialised{false};

  /// Cached bone matrices of the skeleton, including the world transform.
  std::vector<Ogre::Affine3, Affine3Allocator> mBoneWorldMatrices{};
  /// Cached world transform of the parent node.
  Ogre::Affine3 mLastParentXform{Ogre::Affine3::IDENTITY};
  /// State count of the mesh, so that the entity can tell if the mesh changes.
  std::size_t mMeshStateCount{};
  /// AABB including both the mesh's AABB and all the attached objects.
  /// Needed because `getBoundingBox()` wants to return by const reference, and
  /// must be mutable because `getBoundingBox()` wants to be const.
  mutable Ogre::AxisAlignedBox mFullAABB{};

  void buildSubEntityList(const oo::MeshPtr &mesh, SubEntityList &list);

  void updateAnimation();
  void setSkeletonImpl();

  /// \pre `movable` is not attached to this entity
  /// \pre Nothing is attached to the `tagPoint`.
  void attachObjectImpl(Ogre::MovableObject *movable, Ogre::TagPoint *tagPoint);
  /// \pre `movable` is attached to this entity.
  void detachObjectImpl(Ogre::MovableObject *movable);
  void detachAllObjectsImpl();

 public:
  ~Entity() override;

  /// Get the mesh defining the geometry of this entity.
  const oo::MeshPtr &getMesh() const;

  /// Get the constituent subentities of this entity.
  const SubEntityList &getSubEntities() const;

  Entity *clone(const std::string &name);

  /// \name MovableObject overrides
  /// @{

  void _notifyCurrentCamera(Ogre::Camera *camera) override;

  const Ogre::AxisAlignedBox &getBoundingBox() const override;

  const Ogre::AxisAlignedBox &
  getWorldBoundingBox(bool derive = false) const override;

  const Ogre::Sphere &
  getWorldBoundingSphere(bool derive = false) const override;

  float getBoundingRadius() const override;

  void _updateRenderQueue(Ogre::RenderQueue *queue) override;

  const std::string &getMovableType() const override;

  uint32_t getTypeFlags() const override;

  void visitRenderables(Ogre::Renderable::Visitor *visitor,
                        bool debugRenderables = false) override;
  /// @}

  Ogre::AnimationState *getAnimationState(const std::string &name) const;
  bool hasAnimationState(const std::string &name) const;
  Ogre::AnimationStateSet *getAllAnimationStates() const;
  void refreshAvailableAnimationState();
  void _updateAnimation();
  bool _isAnimated() const;
  bool _isSkeletonAnimated() const;

  bool isInitialised() const;
  void _initialise(bool forceReinitialise = false);
  void _deinitialise();

  Ogre::TagPoint *
  attachObjectToBone(const std::string &boneName,
                     Ogre::MovableObject *movable,
                     const Ogre::Quaternion &offsetOrientation = Ogre::Quaternion::IDENTITY,
                     const Ogre::Vector3 &offsetPosition = Ogre::Vector3::ZERO);
  void detachObjectFromBone(Ogre::MovableObject *movable) override;
  Ogre::MovableObject *detachObjectFromBone(const std::string &name);
  /// \remark Note the additional 's' compared to
  ///         `Ogres::Entity::detachAllObjectsFromBone()`.
  void detachAllObjectsFromBones();
  ChildObjectListIterator getAttachedObjectIterator();

  bool hasSkeleton() const;
  Ogre::SkeletonInstance *getSkeleton() const;
  void setSkeleton(const Ogre::SkeletonPtr &skeletonPtr);
  void shareSkeleton(oo::Entity *other);
};

class EntityFactory : public Ogre::MovableObjectFactory {
 public:
  friend class oo::Entity;

  EntityFactory() = default;
  ~EntityFactory() override = default;

  void destroyInstance(gsl::owner<Ogre::MovableObject *> obj) override;
  const std::string &getType() const override;

 protected:
  gsl::owner<Ogre::MovableObject *>
  createInstanceImpl(const std::string &name,
                     const Ogre::NameValuePairList *params) override;

 private:
  constexpr static const char *FACTORY_TYPE_NAME{"Entity"};
};

} // namespace oo

#endif // OPENOBLIVION_ENTITY_HPP
