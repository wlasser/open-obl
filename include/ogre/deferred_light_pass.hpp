#ifndef OPENOBLIVION_OGRE_DEFERRED_LIGHT_PASS_HPP
#define OPENOBLIVION_OGRE_DEFERRED_LIGHT_PASS_HPP

#include <OgreCompositorInstance.h>
#include <OgreCustomCompositionPass.h>
#include <OgreLight.h>
#include <OgreSimpleRenderable.h>

namespace oo {

using RenderOperation = Ogre::CompositorInstance::RenderSystemOperation;

class DeferredLight : public Ogre::SimpleRenderable {
 public:
  explicit DeferredLight(Ogre::Light *parent);
  ~DeferredLight() override;

  Ogre::Real getBoundingRadius() const override;
  Ogre::Real getSquaredViewDepth(const Ogre::Camera *camera) const override;
  void getWorldTransforms(Ogre::Matrix4 *xform) const override;

  const Ogre::AxisAlignedBox &
  getWorldBoundingBox(bool derive = false) const override;
  const Ogre::Sphere &
  getWorldBoundingSphere(bool derive = false) const override;

  Ogre::Light *getParent() const;

  bool isInsideLight(Ogre::Camera *camera) const;

  using LightTypes = Ogre::Light::LightTypes;

  void rebuildLightGeometry();

 private:
  void createPointLight();
  void createDirectionalLight();

  void setPointLightMaterial();
  void setDirectionalLightMaterial();

  Ogre::Light *mParent{};

  /// \name Cached Light Parameters
  /// Several properties of an `Ogre::Light` influence the geometry of the
  /// light mesh, meaning that the geometry must be regenerated when those
  /// properties change. The relevant properties are therefore cached here and
  /// compared against `mParent`'s actual values when the light is to be
  /// rendered, and the geometry updated if they are out of date.
  ///
  /// Deriving from `Ogre::Light` would also work here, but since the property
  /// setters are not virtual a new `update` method (or similar) would need to
  /// added and manually called by the user.
  /// @{
  LightTypes mLightType;
  Ogre::Real mRadius;
  Ogre::Real mAttenConstant;
  Ogre::Real mAttenLinear;
  Ogre::Real mAttenQuadratic;
  /// @}
};

class AmbientLight : public Ogre::SimpleRenderable {
 public:
  AmbientLight();
  ~AmbientLight() override;

  Ogre::Real getBoundingRadius() const override;
  Ogre::Real getSquaredViewDepth(const Ogre::Camera *camera) const override;
  void getWorldTransforms(Ogre::Matrix4 *xform) const override;

 private:
  Ogre::Real mRadius{};
};

class DeferredLightRenderOperation : public RenderOperation {
 public:
  DeferredLightRenderOperation(Ogre::CompositorInstance *instance,
                               const Ogre::CompositionPass *pass);

  ~DeferredLightRenderOperation() override = default;

  void execute(Ogre::SceneManager *scnMgr, Ogre::RenderSystem *rs) override;

 private:
  std::array<std::string, 3u> mTexNames{};
  Ogre::Viewport *mViewport;
  std::unique_ptr<AmbientLight> mAmbientLight{};

  void executeAmbientLight(Ogre::SceneManager *scnMgr);
};

class DeferredLightPass : public Ogre::CustomCompositionPass {
 public:
  ~DeferredLightPass() override = default;

  RenderOperation *createOperation(Ogre::CompositorInstance *instance,
                                   const Ogre::CompositionPass *pass) override {
    return OGRE_NEW oo::DeferredLightRenderOperation(instance, pass);
  }
};

/// Attach this to a post-processing `Ogre::CompositorInstance` to notify it of
/// the scene manager's fog colour and parameters.
/// Fog is rendered as a post-processing effect so does not have access to the
/// scene manager's auto constants and hence needs to be given the fog
/// parameters manually.
class DeferredFogListener : public Ogre::CompositorInstance::Listener {
 public:
  void
  notifyMaterialRender(uint32_t passId, Ogre::MaterialPtr &matPtr) override;

  explicit DeferredFogListener(Ogre::SceneManager *scnMgr) noexcept;

 private:
  Ogre::SceneManager *mScnMgr;
};

} // namespace oo

#endif // OPENOBLIVION_OGRE_DEFERRED_LIGHT_PASS_HPP