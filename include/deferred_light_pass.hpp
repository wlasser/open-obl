#ifndef OPENOBLIVION_DEFERRED_LIGHT_PASS_HPP
#define OPENOBLIVION_DEFERRED_LIGHT_PASS_HPP

#include <OgreCompositorInstance.h>
#include <OgreCustomCompositionPass.h>
#include <OgreSimpleRenderable.h>

namespace oo {

using RenderOperation = Ogre::CompositorInstance::RenderSystemOperation;

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
