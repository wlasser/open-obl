#ifndef OPENOBLIVION_SUBENTITY_HPP
#define OPENOBLIVION_SUBENTITY_HPP

#include <OgreRenderable.h>
#include <OgreMaterial.h>

namespace oo {

class Entity;
class SubMesh;

class SubEntity : public Ogre::Renderable {
 public:
  const std::string &getMaterialName() const;
  void setMaterialName(const std::string &name, const std::string &group);
  void setMaterial(const Ogre::MaterialPtr &material);

  void setVisible(bool visible);
  bool isVisible() const;

  void setRenderQueueGroup(uint8_t queueId);
  void setRenderQueueGroupAndPriority(uint8_t queueId, uint16_t priority);

  uint8_t getRenderQueueGroup() const;
  uint16_t getRenderQueuePriority() const;

  bool isRenderQueueGroupSet() const;
  bool isRenderQueuePrioritySet() const;

  oo::SubMesh *getSubMesh() const;
  oo::Entity *getParent() const;

  /// \name Renderable overrides
  /// @{

  const Ogre::MaterialPtr &getMaterial() const override;
  Ogre::Technique *getTechnique() const override;
  void getRenderOperation(Ogre::RenderOperation &op) override;
  void getWorldTransforms(Ogre::Matrix4 *xform) const override;
  uint16_t getNumWorldTransforms() const override;
  float getSquaredViewDepth(const Ogre::Camera *camera) const override;
  const Ogre::LightList &getLights() const override;
  bool getCastsShadows() const override;

  ///@}

  void _invalidateCameraCache();

 private:
  friend class Entity;

  explicit SubEntity(oo::Entity *parent, oo::SubMesh *subMesh);

  oo::Entity *mParent{};
  oo::SubMesh *mSubMesh{};
  Ogre::MaterialPtr mMaterialPtr{};

  bool mIsVisible : 1;
  /// Whether to use `mRenderQueueId` instead of the default.
  bool mUseCustomRenderQueueId : 1;
  /// Whether to use `mRenderQueuePriority` instead of the default.
  bool mUseCustomRenderQueuePriority : 1;
  /// The render queue to use when rendering this subentity.
  uint8_t mRenderQueueId{};
  /// The render queue priority to use when rendering this subentity.
  uint16_t mRenderQueuePriority{};
  /// Cached distance to distance.
  mutable float mCachedCameraDist{};
  /// Cached camera that `mCachedCameraDist` was measured to. This is
  /// invalidated whenever the camera moves, too.
  mutable const Ogre::Camera *mCachedCamera{};

  /// Cached map translating blend indices to bone indices. This is relative to
  /// the current skeleton of the parent entity and is determined via the bone
  /// names of the submesh.
  std::vector<uint16_t> mBlendIndexToBoneIndexMap{};
};

} // namespace oo

#endif // OPENOBLIVION_SUBENTITY_HPP
