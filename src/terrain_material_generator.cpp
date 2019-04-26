#include "settings.hpp"
#include "terrain_material_generator.hpp"
#include <OgreMaterialManager.h>
#include <OgrePass.h>
#include <OgreTechnique.h>
#include <OgreTerrain.h>
#include <OgreTextureManager.h>

namespace oo {

std::pair<Ogre::MaterialPtr, bool>
TerrainMaterialProfile::createOrRetrieveMaterial(const Ogre::Terrain *terrain) {
  auto &matMgr{Ogre::MaterialManager::getSingleton()};
  const auto numLayers{terrain->getLayerCount()};
  const std::string &matName{terrain->getMaterialName()};

  if (auto matPtr{matMgr.getByName(matName, oo::RESOURCE_GROUP)}) {
    return {std::move(matPtr), false};
  }

  auto baseMat{matMgr.getByName(numLayers <= 5 ? "__LandscapeMaterial5"
                                               : "__LandscapeMaterial9",
                                oo::SHADER_GROUP)};
  return {baseMat->clone(terrain->getMaterialName(), true, oo::RESOURCE_GROUP),
          true};
}

std::pair<Ogre::MaterialPtr, bool>
TerrainMaterialProfile::createOrRetrieveCompositeMaterial(const Ogre::Terrain *terrain) {
  if (auto matPtr{terrain->_getCompositeMapMaterial()}) {
    return {std::move(matPtr), false};
  }

  auto &matMgr{Ogre::MaterialManager::getSingleton()};
  const std::string &matName{terrain->getMaterialName() + "Composite"};
  if (auto matPtr = matMgr.getByName(matName, oo::RESOURCE_GROUP)) {
    return {std::move(matPtr), false};
  }

  auto baseMat{matMgr.getByName("__LandscapeMaterialDiffuseComposite",
                                oo::SHADER_GROUP)};

  return {baseMat->clone(matName, true, oo::RESOURCE_GROUP), true};
}

Ogre::String
TerrainMaterialProfile::createGlobalNormalMap(const Ogre::String &matName) {
  // Leaving the data uninitialized or filling it with zeroes both result in
  // UB if the normals are not populated correctly before rendering; shaders
  // are allowed to assume that the normals are indeed normalized and thus
  // can do things like normalize(n x (nonzero vector)).
  // Alternatively we could fill this with normalized placeholder data---all
  // up vectors for instance---but that seems like a waste.
  auto &texMgr{Ogre::TextureManager::getSingleton()};
  auto globalNormalName{matName + "normal"};
  if (!texMgr.resourceExists(globalNormalName, oo::RESOURCE_GROUP)) {
    texMgr.createManual(
        globalNormalName, oo::RESOURCE_GROUP,
        Ogre::TEX_TYPE_2D, 17u, 17u, 1, 0,
        Ogre::PixelFormat::PF_BYTE_RGB, Ogre::TU_STATIC);
  }
  return globalNormalName;
}

Ogre::String
TerrainMaterialProfile::createVertexColorMap(const Ogre::String &matName) {
  // Filling it with 0 or 255 would at least be a valid default, but there's no
  // need to.
  auto &texMgr{Ogre::TextureManager::getSingleton()};
  auto vertexColorName{matName + "vertexcolor"};
  if (!texMgr.resourceExists(vertexColorName, oo::RESOURCE_GROUP)) {
    texMgr.createManual(
        vertexColorName, oo::RESOURCE_GROUP,
        Ogre::TEX_TYPE_2D, 17u, 17u, 1, 0,
        Ogre::PixelFormat::PF_BYTE_RGB, Ogre::TU_STATIC);
  }
  return vertexColorName;
}

Ogre::String
TerrainMaterialProfile::createCompositeDiffuseMap(const Ogre::String &matName) {
  auto &texMgr{Ogre::TextureManager::getSingleton()};
  const auto &terrainOpts{Ogre::TerrainGlobalOptions::getSingleton()};

  auto compDiffuseName{matName + "compDiffuse"};
  if (!texMgr.resourceExists(compDiffuseName, oo::RESOURCE_GROUP)) {
    texMgr.createManual(
        compDiffuseName, oo::RESOURCE_GROUP, Ogre::TEX_TYPE_2D,
        terrainOpts.getCompositeMapSize(), terrainOpts.getCompositeMapSize(),
        1, 0, Ogre::PixelFormat::PF_BYTE_RGB, Ogre::TU_STATIC);
  }
  return compDiffuseName;
}

Ogre::String
TerrainMaterialProfile::createCompositeNormalMap(const Ogre::String &matName) {
  auto &texMgr{Ogre::TextureManager::getSingleton()};
  const auto &terrainOpts{Ogre::TerrainGlobalOptions::getSingleton()};

  auto compNormalName{matName + "compNormal"};
  if (!texMgr.resourceExists(compNormalName, oo::RESOURCE_GROUP)) {
    texMgr.createManual(
        compNormalName, oo::RESOURCE_GROUP, Ogre::TEX_TYPE_2D,
        terrainOpts.getCompositeMapSize(), terrainOpts.getCompositeMapSize(),
        1, 0, Ogre::PixelFormat::PF_BYTE_RGB, Ogre::TU_STATIC);
  }
  return compNormalName;
}

void TerrainMaterialProfile::makeHighLodTechnique(const Ogre::Terrain *terrain,
                                                  Ogre::Technique *technique) {
  const std::string &matName{terrain->getMaterialName()};
  const auto numLayers{terrain->getLayerCount()};

  auto globalNormalName{createGlobalNormalMap(matName)};
  auto vertexColorName{createVertexColorMap(matName)};

  constexpr auto CLAMP{Ogre::TextureAddressingMode::TAM_CLAMP};
  constexpr auto WRAP{Ogre::TextureAddressingMode::TAM_WRAP};

  if (numLayers <= 5) {
    // Only one pass required.
    auto *pass{technique->getPass(0)};
    pass->removeAllTextureUnitStates();

    auto *globalNormal{pass->createTextureUnitState(globalNormalName)};
    globalNormal->setTextureAddressingMode(CLAMP);

    auto *vertexColor{pass->createTextureUnitState(vertexColorName)};
    vertexColor->setTextureAddressingMode(CLAMP);

    const auto &blendName{terrain->getBlendTextureName(0)};
    auto *blend{pass->createTextureUnitState(blendName)};
    blend->setTextureAddressingMode(CLAMP);

    if (numLayers == 0) return;

    for (uint8_t i = 0; i < 5; ++i) {
      // Compute layer number
      const auto n{std::min<uint8_t>(i, numLayers - 1u)};

      const std::string &diffuseName{terrain->getLayerTextureName(n, 0)};
      auto *diffuse{pass->createTextureUnitState(diffuseName)};
      diffuse->setTextureAddressingMode(WRAP);

      const std::string &normalName{terrain->getLayerTextureName(n, 1)};
      auto *normal{pass->createTextureUnitState(normalName)};
      normal->setTextureAddressingMode(WRAP);
    }
  } else {
    // Two passes required, one pass for the diffuse and one for the normals.
    std::array<std::string, 2u> baseMapNames{
        vertexColorName,
        globalNormalName
    };
    for (uint8_t passIndex = 0; passIndex < 2; ++passIndex) {
      auto *pass{technique->getPass(passIndex)};
      pass->removeAllTextureUnitStates();

      auto *baseMap{pass->createTextureUnitState(baseMapNames[passIndex])};
      baseMap->setTextureAddressingMode(CLAMP);

      const auto &blend0Name{terrain->getBlendTextureName(0)};
      auto *blend0{pass->createTextureUnitState(blend0Name)};
      blend0->setTextureAddressingMode(CLAMP);

      const auto &blend1Name{terrain->getBlendTextureName(1)};
      auto *blend1{pass->createTextureUnitState(blend1Name)};
      blend1->setTextureAddressingMode(CLAMP);

      for (uint8_t i = 0; i < 9; ++i) {
        const auto n{std::min<uint8_t>(i, numLayers - 1u)};

        const auto &layerName{terrain->getLayerTextureName(n, passIndex)};
        auto *layer{pass->createTextureUnitState(layerName)};
        layer->setTextureAddressingMode(WRAP);
      }
    }

  }
}

void TerrainMaterialProfile::makeLowLodTechnique(const Ogre::Terrain *terrain,
                                                 Ogre::Technique *technique) {
  const std::string &matName{terrain->getMaterialName()};
  auto *pass{technique->getPass(0)};
  pass->removeAllTextureUnitStates();

  constexpr auto CLAMP{Ogre::TextureAddressingMode::TAM_CLAMP};

  auto compDiffuseName{terrain->getCompositeMap()->getName()};
  auto *compDiffuse{pass->createTextureUnitState(compDiffuseName)};
  compDiffuse->setTextureAddressingMode(CLAMP);

//  auto compNormalName{createCompositeNormalMap(matName)};
  auto compNormalName{createGlobalNormalMap(matName)};
  auto *compNormal{pass->createTextureUnitState(compNormalName)};
  compNormal->setTextureAddressingMode(CLAMP);
}

void TerrainMaterialProfile::makeDiffuseCompositeTechnique(const Ogre::Terrain *terrain,
                                                           Ogre::Technique *technique) {
  const std::string &matName{terrain->getMaterialName()};
  auto *pass{technique->getPass(0)};
  pass->removeAllTextureUnitStates();

  constexpr auto CLAMP{Ogre::TextureAddressingMode::TAM_CLAMP};
  constexpr auto WRAP{Ogre::TextureAddressingMode::TAM_WRAP};

  const auto numLayers{terrain->getLayerCount()};

  pass->getFragmentProgramParameters()->setNamedConstant(
      "numBlendMaps", numLayers <= 5 ? 1 : 2);

  auto vertexColorName{createVertexColorMap(matName)};
  auto *vertexColor{pass->createTextureUnitState(vertexColorName)};
  vertexColor->setTextureAddressingMode(CLAMP);

  const auto &blendMap0Name{terrain->getBlendTextureName(0)};
  auto *blendMap0{pass->createTextureUnitState(blendMap0Name)};
  blendMap0->setTextureAddressingMode(CLAMP);

  const auto &blendMap1Name{numLayers <= 5 ? blendMap0Name
                                           : terrain->getBlendTextureName(1)};
  auto *blendMap1{pass->createTextureUnitState(blendMap1Name)};
  blendMap1->setTextureAddressingMode(CLAMP);

  // Every diffuse map is assigned to. If we don't have enough layers then the
  // last layer is used for each subsequent diffuse map.
  for (uint8_t i = 0; i < 9; ++i) {
    const auto n{std::min(i, static_cast<uint8_t>(numLayers - 1u))};

    const std::string &diffuseName{terrain->getLayerTextureName(n, 0)};
    auto *diffuse{pass->createTextureUnitState(diffuseName)};
    diffuse->setTextureAddressingMode(WRAP);
  }
}

Ogre::MaterialPtr
TerrainMaterialProfile::generate(const Ogre::Terrain *terrain) {
  auto matPtr{createOrRetrieveMaterial(terrain).first};

  Ogre::Material::LodValueList lodValues{
      Ogre::TerrainGlobalOptions::getSingleton().getCompositeMapDistance()
  };
  matPtr->setLodLevels(lodValues);

  makeHighLodTechnique(terrain, matPtr->getTechnique("HighLOD"));
  if (terrain->isLoaded()) {
    makeLowLodTechnique(terrain, matPtr->getTechnique("LowLOD"));
  }

  return matPtr;
}

Ogre::MaterialPtr
TerrainMaterialProfile::generateForCompositeMap(const Ogre::Terrain *terrain) {
  if (!terrain->isLoaded()) return Ogre::MaterialPtr();

  auto matPtr{createOrRetrieveCompositeMaterial(terrain).first};

  makeDiffuseCompositeTechnique(terrain, matPtr->getTechnique(0));

  return matPtr;
}

uint8_t TerrainMaterialProfile::getMaxLayers(const Ogre::Terrain *) const {
  return 9u;
}

bool TerrainMaterialProfile::isVertexCompressionSupported() const {
  return false;
}

void TerrainMaterialProfile::requestOptions(Ogre::Terrain *terrain) {
  terrain->_setMorphRequired(false);
  terrain->_setNormalMapRequired(false);
  terrain->_setLightMapRequired(false);
  terrain->_setCompositeMapRequired(terrain->isLoaded());
}

void TerrainMaterialProfile::setLightmapEnabled(bool) {
  // Lightmaps are not supported.
}

void TerrainMaterialProfile::updateParams(const Ogre::MaterialPtr &,
                                          const Ogre::Terrain *) {
  // Dynamic terrain is not supported so this is unnecessary.
}

void
TerrainMaterialProfile::updateParamsForCompositeMap(const Ogre::MaterialPtr &,
                                                    const Ogre::Terrain *) {
  // Dynamic terrain is not supported so this is unnecessary.
}

TerrainMaterialGenerator::TerrainMaterialGenerator() {
  const std::string profileName{"default"};
  // This allocation is expected by the base class, which will clean up.
  mProfiles.push_back(OGRE_NEW oo::TerrainMaterialProfile(
      this, profileName, "Default profile"));

  setActiveProfile(profileName);

  auto &elems{mDecl.elements};
  elems.emplace_back(0, Ogre::TerrainLayerSamplerSemantic::TLSS_ALBEDO, 0, 3);
  elems.emplace_back(1, Ogre::TerrainLayerSamplerSemantic::TLSS_NORMAL, 0, 4);

  auto &samplers{mDecl.samplers};
  samplers.emplace_back("diffuse", Ogre::PixelFormat::PF_BYTE_RGB);
  samplers.emplace_back("normal", Ogre::PixelFormat::PF_BYTE_RGBA);
}

const Ogre::TerrainLayerDeclaration &
TerrainMaterialGenerator::getLayerDeclaration() const {
  return mDecl;
}

} // namespace oo