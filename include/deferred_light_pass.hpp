#ifndef OPENOBLIVION_DEFERRED_LIGHT_PASS_HPP
#define OPENOBLIVION_DEFERRED_LIGHT_PASS_HPP

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

  Ogre::Light *getParent() const;

  bool isInsideLight(Ogre::Camera *camera) const;

  using LightTypes = Ogre::Light::LightTypes;

  void rebuildLightGeometry();

 private:
  void createPointLight();

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

} // namespace oo

#endif // OPENOBLIVION_DEFERRED_LIGHT_PASS_HPP
