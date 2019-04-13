#include "deferred_light_pass.hpp"

#include <OgreCompositorChain.h>
#include <OgreHardwareBufferManager.h>
#include <OgrePass.h>
#include <OgreSceneManager.h>
#include <OgreTechnique.h>

namespace oo {

AmbientLight::AmbientLight() {
  setRenderQueueGroup(Ogre::RENDER_QUEUE_2);
  mRenderOp.vertexData = OGRE_NEW Ogre::VertexData();
  mRenderOp.vertexData->vertexCount = 4u;
  mRenderOp.vertexData->vertexStart = 0u;
  mRenderOp.indexData = nullptr;

  auto *vertDecl{mRenderOp.vertexData->vertexDeclaration};
  auto *vertBind{mRenderOp.vertexData->vertexBufferBinding};

  vertDecl->addElement(0, 0, Ogre::VET_FLOAT2, Ogre::VES_POSITION);

  auto &hwBufMgr{Ogre::HardwareBufferManager::getSingleton()};
  auto bufPtr{hwBufMgr.createVertexBuffer(vertDecl->getVertexSize(0), 4u,
                                          Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY)};
  vertBind->setBinding(0, bufPtr);
  std::array<float, 4u * 2u> vertices{
      -1, 1,
      -1, -1,
      1, 1,
      1, -1,
  };
  bufPtr->writeData(0, vertices.size() * sizeof(float), vertices.data(), true);

  mRenderOp.operationType = Ogre::RenderOperation::OT_TRIANGLE_STRIP;
  mRenderOp.useIndexes = false;

  setBoundingBox(Ogre::AxisAlignedBox(-5000.0f, -5000.0f, -5000.0f,
                                      5000.0f, 5000.0f, 5000.0f));
  mRadius = 8000.0f;

  auto &matMgr{Ogre::MaterialManager::getSingleton()};
  mMaterial = matMgr.getByName("DeferredAmbient");
  mMaterial->load();

  auto *pass{mMaterial->getTechnique(0)->getPass(0)};
  const auto &params{pass->getFragmentProgramParameters()};
  params->setNamedConstant("Tex0", 0);
  params->setNamedConstant("Tex1", 1);
  params->setNamedConstant("Tex2", 2);
}

AmbientLight::~AmbientLight() {
  OGRE_DELETE mRenderOp.vertexData;
  OGRE_DELETE mRenderOp.indexData;
}

Ogre::Real AmbientLight::getBoundingRadius() const {
  return mRadius;
}

Ogre::Real AmbientLight::getSquaredViewDepth(const Ogre::Camera *) const {
  return 0.0f;
}

void AmbientLight::getWorldTransforms(Ogre::Matrix4 *xform) const {
  *xform = Ogre::Matrix4::IDENTITY;
}

DeferredLightRenderOperation::DeferredLightRenderOperation(Ogre::CompositorInstance *instance,
                                                           const Ogre::CompositionPass *pass)
    : mViewport(instance->getChain()->getViewport()),
      mAmbientLight(std::make_unique<oo::AmbientLight>()) {
  for (std::size_t i = 0; i < mTexNames.size(); ++i) {
    const auto &input{pass->getInput(i)};
    mTexNames[i] = instance->getTextureInstanceName(input.name, input.mrtIndex);
  }
}

void DeferredLightRenderOperation::execute(Ogre::SceneManager *scnMgr,
                                           Ogre::RenderSystem *rs) {
  Ogre::Camera *camera{mViewport->getCamera()};
  auto *technique{mAmbientLight->getMaterial()->getBestTechnique()};

  for (auto *pass : technique->getPasses()) {
    scnMgr->_injectRenderWithPass(pass, mAmbientLight.get(), false);
  }
}

} // namespace oo