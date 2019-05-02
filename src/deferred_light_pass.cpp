#include "deferred_light_pass.hpp"
#include "math/conversions.hpp"
#include "scene_manager.hpp"
#include "settings.hpp"

#include <OgreCamera.h>
#include <OgreCompositorChain.h>
#include <OgreHardwareBufferManager.h>
#include <OgrePass.h>
#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreTechnique.h>

namespace oo {

DeferredLight::DeferredLight(Ogre::Light *parent)
    : mParent(parent),
      mLightType(parent->getType()),
      mRadius(parent->getAttenuationRange()),
      mAttenConstant(parent->getAttenuationConstant()),
      mAttenLinear(parent->getAttenuationLinear()),
      mAttenQuadratic(parent->getAttenuationQuadric()) {
  mRenderOp.vertexData = nullptr;
  mRenderOp.indexData = nullptr;
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

bool DeferredLight::isInsideLight(Ogre::Camera *camera) const {
  switch (mParent->getType()) {
    case LightTypes::LT_POINT: {
      const auto &p1{camera->getDerivedPosition()};
      const auto &p2{mParent->getDerivedPosition()};
      return p1.squaredDistance(p2) <= mRadius * mRadius;
    }
    case LightTypes::LT_DIRECTIONAL:return false;
    case LightTypes::LT_SPOTLIGHT:return false;
    default:return false;
  }
}

void DeferredLight::rebuildLightGeometry() {
  const auto lightType{mParent->getType()};
  const auto radius{mParent->getAttenuationRange()};

  if (mLightType != lightType
      || (radius != mRadius && lightType != LightTypes::LT_DIRECTIONAL)) {
    OGRE_DELETE mRenderOp.vertexData;
    OGRE_DELETE mRenderOp.indexData;

    // For exception safety these should really be set after the light has been
    // rebuilt, but they need to be correct for createPointLight to work.
    mLightType = lightType;
    mRadius = radius;

    switch (lightType) {
      case LightTypes::LT_POINT: {
        setBoundingBox(Ogre::AxisAlignedBox(-radius, -radius, -radius,
                                            radius, radius, radius));
        createPointLight();
        setPointLightMaterial();
        break;
      }
      case LightTypes::LT_DIRECTIONAL: {
        setBoundingBox(Ogre::AxisAlignedBox(-5000.0f, -5000.0f, -5000.0f,
                                            5000.0f, 5000.0f, 5000.0f));
        mRadius = 8000.0f;
        createDirectionalLight();
        setDirectionalLightMaterial();
        break;
      }
      case LightTypes::LT_SPOTLIGHT: return;
      default: return;
    }
  }
}

const Ogre::AxisAlignedBox &
DeferredLight::getWorldBoundingBox(bool derive) const {
  if (derive) {
    mWorldAABB = getBoundingBox();
    mWorldAABB.transform(mParent->_getParentNodeFullTransform());
  }

  return mWorldAABB;
}

const Ogre::Sphere &
DeferredLight::getWorldBoundingSphere(bool derive) const {
  if (derive) {
    auto scale{mParent->getParentNode()->_getDerivedScale()};
    auto max{std::max(qvm::X(scale), std::max(qvm::Y(scale), qvm::Z(scale)))};
    auto pos{mParent->getParentNode()->_getDerivedPosition()};
    mWorldBoundingSphere.setRadius(max * getBoundingRadius());
    mWorldBoundingSphere.setCenter(pos);
  }

  return mWorldBoundingSphere;
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
  const float sf{mRadius / (Ogre::Math::Sqrt(2.0f + phi))};
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

void DeferredLight::createDirectionalLight() {
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
      -1, -1,
      1, -1,
      -1, 1,
      1, 1,
  };
  bufPtr->writeData(0, vertices.size() * sizeof(float), vertices.data(), true);

  mRenderOp.operationType = Ogre::RenderOperation::OT_TRIANGLE_STRIP;
  mRenderOp.useIndexes = false;
}

void DeferredLight::setPointLightMaterial() {
  auto &matMgr{Ogre::MaterialManager::getSingleton()};
  mMaterial = matMgr.getByName("DeferredPointLight");
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

void DeferredLight::setDirectionalLightMaterial() {
  auto &matMgr{Ogre::MaterialManager::getSingleton()};
  mMaterial = matMgr.getByName("DeferredDirectionalLight");
  mMaterial->load();

  auto *pass{mMaterial->getTechnique(0)->getPass(0)};
  const auto &params{pass->getFragmentProgramParameters()};
  params->setNamedConstant("Tex0", 0);
  params->setNamedConstant("Tex1", 1);
  params->setNamedConstant("Tex2", 2);

  using AutoConst = Ogre::GpuProgramParameters::AutoConstantType;
  params->setNamedAutoConstant("ViewPos",
                               AutoConst::ACT_CAMERA_POSITION);
  params->setNamedAutoConstant("lightDirection",
                               AutoConst::ACT_LIGHT_POSITION);
  params->setNamedAutoConstant("lightDiffuseCol",
                               AutoConst::ACT_LIGHT_DIFFUSE_COLOUR);
  params->setNamedAutoConstant("lightAttenuation",
                               AutoConst::ACT_LIGHT_ATTENUATION);

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
      -1, -1,
      1, -1,
      -1, 1,
      1, 1,
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

void DeferredLightRenderOperation::executeAmbientLight(Ogre::SceneManager *scnMgr) {
  auto *technique{mAmbientLight->getMaterial()->getBestTechnique()};
  if (!technique) return;

  for (auto *pass : technique->getPasses()) {
    scnMgr->_injectRenderWithPass(pass, mAmbientLight.get(), false);
  }
}

void DeferredLightRenderOperation::execute(Ogre::SceneManager *scnMgr,
                                           Ogre::RenderSystem *rs) {
  executeAmbientLight(scnMgr);

  auto *dScnMgr{dynamic_cast<oo::DeferredSceneManager *>(scnMgr)};
  if (!dScnMgr) return;

  Ogre::Camera *camera{mViewport->getCamera()};
  const auto dLights{dScnMgr->getLights()};
  const Ogre::LightList &lights{dScnMgr->_getLightsAffectingFrustum()};

  for (auto *light : lights) {
    auto pred = [light](auto *l) { return l->getParent() == light; };
    auto it{std::find_if(dLights.begin(), dLights.end(), pred)};
    if (it == dLights.end()) continue;
    auto *dLight{*it};

    if (light->getType() == Ogre::Light::LightTypes::LT_SPOTLIGHT) continue;

    // rebuildLightGeometry() may update material params so must do that before
    // getting the technique.
    dLight->rebuildLightGeometry();

    auto *technique{dLight->getMaterial()->getBestTechnique()};
    if (!technique) continue;

    for (auto *pass : technique->getPasses()) {
      if (light->getType() != Ogre::Light::LightTypes::LT_DIRECTIONAL) {
        bool isInside{dLight->isInsideLight(camera)};
        if (isInside) {
          pass->setCullingMode(Ogre::CullingMode::CULL_ANTICLOCKWISE);
          pass->setDepthFunction(Ogre::CompareFunction::CMPF_GREATER_EQUAL);
        } else {
          pass->setCullingMode(Ogre::CullingMode::CULL_CLOCKWISE);
          pass->setDepthFunction(Ogre::CompareFunction::CMPF_LESS_EQUAL);
        }
      }

      // LightList does not support construction from std::initializer_list :(
      Ogre::LightList dLightList(1, light);
      dScnMgr->_injectRenderWithPass(pass, *it, false, false, &dLightList);
    }

  }
}

DeferredFogListener::DeferredFogListener(Ogre::SceneManager *scnMgr) noexcept
    : mScnMgr(scnMgr) {}

void
DeferredFogListener::notifyMaterialRender(uint32_t, Ogre::MaterialPtr &matPtr) {
  auto *postPass{matPtr->getTechnique(0)->getPass(0)};
  auto postParams{postPass->getFragmentProgramParameters()};
  postParams->setNamedConstant("fogColor", mScnMgr->getFogColour());
  postParams->setNamedConstant("fogParams", Ogre::Vector4{
      mScnMgr->getFogDensity(),
      mScnMgr->getFogStart(),
      mScnMgr->getFogEnd(),
      1.0f / (mScnMgr->getFogEnd() - mScnMgr->getFogStart())
  });
}

} // namespace oo