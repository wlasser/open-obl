#include "settings.hpp"
#include "terrain_material_generator.hpp"
#include <OgreMaterialManager.h>
#include <OgrePass.h>
#include <OgreTechnique.h>
#include <OgreTerrain.h>
#include <OgreTextureManager.h>

namespace oo {

Ogre::MaterialPtr
TerrainMaterialProfile::generate(const Ogre::Terrain *terrain) {
  auto &matMgr{Ogre::MaterialManager::getSingleton()};
  auto &texMgr{Ogre::TextureManager::getSingleton()};

  const auto numLayers{terrain->getLayerCount()};

  const std::string &matName{terrain->getMaterialName()};
  auto matPtr{matMgr.getByName(matName, oo::RESOURCE_GROUP)};
  if (!matPtr) {
    auto baseMat{matMgr.getByName(numLayers <= 5 ? "__LandscapeMaterial5"
                                                 : "__LandscapeMaterial9",
                                  oo::SHADER_GROUP)};
    matPtr = baseMat->clone(terrain->getMaterialName(),
                            true, oo::RESOURCE_GROUP);
  }

  auto *basePass{matPtr->getTechnique(0)->getPass(0)};
  if (basePass->getNumTextureUnitStates() > 0) return matPtr;

  // The global normal map's name is dependent on the Terrain pointer, which
  // is not available until the terrain is loaded. This Material returned by
  // this function is required to reference the global normal map, but this
  // function is called during load, and thus we have to create the map here
  // even though we cannot populate it.
  // Leaving the data uninitialized or filling it with zeroes both result in
  // UB if the normals are not populated correctly before rendering; shaders
  // are allowed to assume that the normals are indeed normalized and thus
  // can do things like normalize(n x (nonzero vector)).
  // Alternatively we could fill this with normalized placeholder data---all
  // up vectors for instance---but that seems like a waste.
  auto globalNormalName{matName + "normal"};
  if (!texMgr.resourceExists(globalNormalName, oo::RESOURCE_GROUP)) {
    texMgr.createManual(
        globalNormalName, oo::RESOURCE_GROUP,
        Ogre::TEX_TYPE_2D, 17u, 17u, 1, 0,
        Ogre::PixelFormat::PF_BYTE_RGB, Ogre::TU_STATIC);
  }

  // Similarly to the global normal map, we need to create the vertex colour
  // map here. Filling it with 0 or 255 would at least be a valid default,
  // but there's no need to.
  auto vertexColorName{matName + "vertexcolor"};
  if (!texMgr.resourceExists(vertexColorName, oo::RESOURCE_GROUP)) {
    texMgr.createManual(
        vertexColorName, oo::RESOURCE_GROUP,
        Ogre::TEX_TYPE_2D, 17u, 17u, 1, 0,
        Ogre::PixelFormat::PF_BYTE_RGB, Ogre::TU_STATIC);
  }

  // WTF C++, why is this cast even necessary? You can *see* 1 and 2 fit in
  // a short, the standard requires it! Using = just shifts changes error to a
  // warning BTW.
  // TODO: Make a short integer UDL.
  const uint8_t numPasses{static_cast<uint8_t>(numLayers <= 5 ? 1u : 2u)};

  for (uint8_t passNumber = 0; passNumber < numPasses; ++passNumber) {
    constexpr auto CLAMP{Ogre::TextureAddressingMode::TAM_CLAMP};
    constexpr auto WRAP{Ogre::TextureAddressingMode::TAM_WRAP};
    auto *pass{matPtr->getTechnique(0)->getPass(passNumber)};

    auto *globalNormal{pass->createTextureUnitState(globalNormalName)};
    globalNormal->setTextureAddressingMode(CLAMP);

    auto *vertexColor{pass->createTextureUnitState(vertexColorName)};
    vertexColor->setTextureAddressingMode(CLAMP);

    const auto &blendName{terrain->getBlendTextureName(passNumber)};
    auto *blend{pass->createTextureUnitState(blendName)};
    blend->setTextureAddressingMode(CLAMP);

    if (numLayers == 0) return matPtr;

    // Base texture
    if (passNumber == 0) {
      const std::string &diffuseName{terrain->getLayerTextureName(0, 0)};
      auto *diffuse{pass->createTextureUnitState(diffuseName)};
      diffuse->setTextureAddressingMode(WRAP);

      const std::string &normalName{terrain->getLayerTextureName(0, 1)};
      auto *normal{pass->createTextureUnitState(normalName)};
      normal->setTextureAddressingMode(WRAP);
    }

    constexpr uint8_t MAX_LAYERS{4u};
    for (uint8_t i = 0; i < MAX_LAYERS; ++i) {
      const auto layerNum{static_cast<uint8_t>(std::min(
          MAX_LAYERS * passNumber + i + 1u,
          numLayers - 1u))};

      const std::string &diffuseName{terrain->getLayerTextureName(layerNum, 0)};
      auto *diffuse{pass->createTextureUnitState(diffuseName)};
      diffuse->setTextureAddressingMode(WRAP);

      const std::string &normalName{terrain->getLayerTextureName(layerNum, 1)};
      auto *normal{pass->createTextureUnitState(normalName)};
      normal->setTextureAddressingMode(WRAP);
    }
  }

  return matPtr;
}

Ogre::MaterialPtr
TerrainMaterialProfile::generateForCompositeMap(const Ogre::Terrain *terrain) {
  return generate(terrain);
}

uint8_t TerrainMaterialProfile::getMaxLayers(const Ogre::Terrain *) const {
  return 3u;
}

bool TerrainMaterialProfile::isVertexCompressionSupported() const {
  return false;
}

void TerrainMaterialProfile::requestOptions(Ogre::Terrain *terrain) {
  terrain->_setMorphRequired(false);
  // We generate our own normal map because Ogre's is in the wrong group and
  // we have explicit normal information anyway.
  terrain->_setNormalMapRequired(false);
  terrain->_setLightMapRequired(false);
  terrain->_setCompositeMapRequired(false);
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
  elems.emplace_back(1, Ogre::TerrainLayerSamplerSemantic::TLSS_NORMAL, 0, 3);

  auto &samplers{mDecl.samplers};
  samplers.emplace_back("diffuse", Ogre::PixelFormat::PF_BYTE_RGB);
  samplers.emplace_back("normal", Ogre::PixelFormat::PF_BYTE_RGB);
}

const Ogre::TerrainLayerDeclaration &
TerrainMaterialGenerator::getLayerDeclaration() const {
  return mDecl;
}

} // namespace oo