#include "deferred_light_pass.hpp"
#include "scene_manager.hpp"

#include <OgreCamera.h>
#include <OgreCompositorChain.h>
#include <OgreHardwareBufferManager.h>
#include <OgrePass.h>
#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreTechnique.h>

namespace oo {

DeferredLight::DeferredLight(Ogre::Light *parent) : mParent(parent) {
  mRenderOp.vertexData = nullptr;
  mRenderOp.indexData = nullptr;

  switch (parent->getType()) {
    case Ogre::Light::LightTypes::LT_POINT: {
      mRadius = parent->getAttenuationRange();
      setBoundingBox(Ogre::AxisAlignedBox(mRadius, mRadius, mRadius,
                                          -mRadius, -mRadius, -mRadius));
      createPointLight();
      break;
    }
    case Ogre::Light::LightTypes::LT_DIRECTIONAL: return;
    case Ogre::Light::LightTypes::LT_SPOTLIGHT: return;
    default: return;
  }

  auto &matMgr{Ogre::MaterialManager::getSingleton()};
  mMaterial = matMgr.getByName("DeferredLight");
  mMaterial->load();

  auto *pass{mMaterial->getTechnique(0)->getPass(0)};
  const auto &params{pass->getFragmentProgramParameters()};
  params->setNamedConstant("Tex0", 0);
  params->setNamedConstant("Tex1", 1);
  params->setNamedConstant("Tex2", 2);

  using AutoConst = Ogre::GpuProgramParameters::AutoConstantType;
  params->setNamedAutoConstant("ViewPos",
                               AutoConst::ACT_CAMERA_POSITION);
  params->setNamedAutoConstant("lightPosition",
                               AutoConst::ACT_LIGHT_POSITION);
  params->setNamedAutoConstant("lightDiffuseCol",
                               AutoConst::ACT_LIGHT_DIFFUSE_COLOUR);
  params->setNamedAutoConstant("lightAttenuation",
                               AutoConst::ACT_LIGHT_ATTENUATION);
}

DeferredLight::~DeferredLight() {
  OGRE_DELETE mRenderOp.vertexData;
  OGRE_DELETE mRenderOp.indexData;
}

Ogre::Real DeferredLight::getBoundingRadius() const {
  return mRadius;
}

Ogre::Real DeferredLight::getSquaredViewDepth(const Ogre::Camera *camera) const {
  auto vec{camera->getDerivedPosition()
               - mParent->getParentSceneNode()->_getDerivedPosition()};
  return vec.squaredLength();
}

void DeferredLight::getWorldTransforms(Ogre::Matrix4 *xform) const {
  xform->makeTransform(mParent->getDerivedPosition(), Ogre::Vector3::UNIT_SCALE,
                       Ogre::Quaternion::IDENTITY);
}

Ogre::Light *DeferredLight::getParent() const {
  return mParent;
}

void DeferredLight::createPointLight() {
  const float phi{(1.0f + Ogre::Math::Sqrt(5)) / 2.0f};
  std::array<float, 12u * 3u> vertices{
      1.0f, 0.0f, phi,
      -1.0f, 0.0f, phi,
      -1.0f, 0.0f, -phi,
      1.0f, 0.0f, -phi,
      0.0f, phi, 1.0f,
      0.0f, phi, -1.0f,
      0.0f, -phi, -1.0f,
      0.0f, -phi, 1.0f,
      phi, 1.0f, 0.0f,
      phi, -1.0f, 0.0f,
      -phi, -1.0f, 0.0f,
      -phi, 1.0f, 0.0f,
  };
  std::array<uint16_t, 20u * 3u> indices{
      8, 9, 3,
      3, 9, 6,
      6, 9, 7,
      7, 9, 0,
      0, 9, 8,

      8, 4, 0,
      0, 4, 1,
      0, 1, 7,
      7, 1, 10,
      7, 10, 6,
      6, 10, 2,
      2, 3, 6,
      2, 5, 3,
      5, 4, 8,
      8, 3, 5,

      5, 11, 4,
      4, 11, 1,
      1, 11, 10,
      10, 11, 2,
      2, 11, 5
  };

  mRenderOp.vertexData = OGRE_NEW Ogre::VertexData();
  mRenderOp.vertexData->vertexCount = 12u;
  mRenderOp.vertexData->vertexStart = 0u;
  mRenderOp.indexData = OGRE_NEW Ogre::IndexData();
  mRenderOp.useIndexes = true;
  mRenderOp.operationType = Ogre::RenderOperation::OT_TRIANGLE_LIST;

  auto &hwBufMgr{Ogre::HardwareBufferManager::getSingleton()};

  auto *vertDecl{mRenderOp.vertexData->vertexDeclaration};
  auto *vertBind{mRenderOp.vertexData->vertexBufferBinding};

  vertDecl->addElement(0, 0, Ogre::VET_FLOAT3, Ogre::VES_POSITION);
  auto vertBuf{hwBufMgr.createVertexBuffer(vertDecl->getVertexSize(0),
                                           12u,
                                           Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY)};
  vertBind->setBinding(0, vertBuf);
//  const float sf{mRadius / (Ogre::Math::Sqrt(2.0 + phi))};
  const float sf{1.0f / (Ogre::Math::Sqrt(2.0f + phi))};
  for (auto &v : vertices) v *= sf;
  vertBuf->writeData(0u, vertices.size() * 4u, vertices.data(), true);

  mRenderOp.indexData->indexCount = 60u;
  mRenderOp.indexData->indexBuffer = hwBufMgr.createIndexBuffer(
      Ogre::HardwareIndexBuffer::IT_16BIT,
      mRenderOp.indexData->indexCount,
      Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY);
  auto &idxBuf{mRenderOp.indexData->indexBuffer};
  idxBuf->writeData(0u, indices.size() * 2u, indices.data(), true);
}

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
//  params->setNamedConstant("Tex0", 0);
//  params->setNamedConstant("Tex1", 1);
//  params->setNamedConstant("Tex2", 2);
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
  if (!technique) return;

  for (auto *pass : technique->getPasses()) {
    scnMgr->_injectRenderWithPass(pass, mAmbientLight.get(), false);
  }

  auto *dScnMgr{dynamic_cast<oo::DeferredSceneManager *>(scnMgr)};
  if (!dScnMgr) return;

  const auto dLights{dScnMgr->getLights()};
  const Ogre::LightList &lights{dScnMgr->_getLightsAffectingFrustum()};
  for (auto *light : lights) {
    auto it{std::find_if(dLights.begin(), dLights.end(), [light](auto *l) {
      return l->getParent() == light;
    })};
    if (it == dLights.end()) continue;

    Ogre::LightList dLightList;
    dLightList.push_back(light);
    technique = (*it)->getMaterial()->getBestTechnique();
    if (!technique) return;

    for (auto *pass : technique->getPasses()) {
      dScnMgr->_injectRenderWithPass(pass, *it, false, false, &dLightList);
    }
  }
}

} // namespace oo