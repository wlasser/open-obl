#ifndef OPENOBLIVION_TERRAIN_MATERIAL_GENERATOR_HPP
#define OPENOBLIVION_TERRAIN_MATERIAL_GENERATOR_HPP

#include "settings.hpp"
#include <OgreMaterialManager.h>
#include <OgreTextureManager.h>
#include <OGRE/Terrain/OgreTerrainMaterialGenerator.h>
#include <spdlog/spdlog.h>

namespace oo {

class TerrainMaterialProfile : public Ogre::TerrainMaterialGenerator::Profile {
 public:
  using Profile = Ogre::TerrainMaterialGenerator::Profile;
  using ProfileList = Ogre::TerrainMaterialGenerator::ProfileList;

  explicit TerrainMaterialProfile(Ogre::TerrainMaterialGenerator *parent,
                                  const Ogre::String &name,
                                  const Ogre::String &desc)
      : Profile(parent, name, desc) {}

  Ogre::MaterialPtr generate(const Ogre::Terrain *terrain) override {
    auto &matMgr{Ogre::MaterialManager::getSingleton()};
    auto &texMgr{Ogre::TextureManager::getSingleton()};

    const std::string &matName{terrain->getMaterialName()};
    auto matPtr{matMgr.getByName(matName, oo::RESOURCE_GROUP)};
    if (!matPtr) {
      auto baseMat{matMgr.getByName("__LandscapeMaterial", oo::SHADER_GROUP)};
      matPtr = baseMat->clone(terrain->getMaterialName(),
                              true, oo::RESOURCE_GROUP);
    }

    auto *pass{matPtr->getTechnique(0)->getPass(0)};
    if (pass->getNumTextureUnitStates() > 0) return matPtr;

    spdlog::get(oo::LOG)->info("Terrain has {} layers",
                               terrain->getLayerCount());

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
          Ogre::TEX_TYPE_2D, 33u, 33u, 1, 0,
          Ogre::PixelFormat::PF_BYTE_RGB, Ogre::TU_STATIC);
    }

    // Similarly to the global normal map, we need to create the vertex colour
    // map here. Filling it with 0 or 255 would at least be a valid default,
    // but there's no need to.
    auto vertexColorName{matName + "vertexcolor"};
    if (!texMgr.resourceExists(vertexColorName, oo::RESOURCE_GROUP)) {
      texMgr.createManual(
          vertexColorName, oo::RESOURCE_GROUP,
          Ogre::TEX_TYPE_2D, 33u, 33u, 1, 0,
          Ogre::PixelFormat::PF_BYTE_RGB, Ogre::TU_STATIC);
    }

    auto *globalNormalState{pass->createTextureUnitState(globalNormalName)};
    auto *vertexColorState{pass->createTextureUnitState(vertexColorName)};

    for (uint8_t i = 0; i < 2; ++i) {
      const std::string &diffuseName{terrain->getLayerTextureName(i, 0)};
      auto *diffuseState{pass->createTextureUnitState(diffuseName)};

      const std::string &normalName{terrain->getLayerTextureName(i, 1)};
      auto *normalState{pass->createTextureUnitState(normalName)};
    }

    return matPtr;
  }

  Ogre::MaterialPtr generateForCompositeMap(const Ogre::Terrain *terrain) override {
    return generate(terrain);
  }

  uint8_t getMaxLayers(const Ogre::Terrain *terrain) const override {
    return 1u;
  }

  bool isVertexCompressionSupported() const override {
    return false;
  }

  void requestOptions(Ogre::Terrain *terrain) override {
    terrain->_setMorphRequired(false);
    // We generate our own normal map because Ogre's is in the wrong group and
    // we have explicit normal information anyway.
    terrain->_setNormalMapRequired(false);
    terrain->_setLightMapRequired(false);
    terrain->_setCompositeMapRequired(false);
  }

  void setLightmapEnabled(bool enabled) override {}

  void updateParams(const Ogre::MaterialPtr &mat,
                    const Ogre::Terrain *terrain) override {}

  void updateParamsForCompositeMap(const Ogre::MaterialPtr &mat,
                                   const Ogre::Terrain *terrain) override {}
};

class TerrainMaterialGenerator : public Ogre::TerrainMaterialGenerator {
 public:
  TerrainMaterialGenerator() {
    // This allocation is expected by the base class, which will clean up.
    mProfiles.push_back(OGRE_NEW oo::TerrainMaterialProfile(
        this, "TerrainMaterialProfile", "Default profile"));

    setActiveProfile("TerrainMaterialProfile");
  }

  ~TerrainMaterialGenerator() override = default;

};

} // namespace oo

#endif // OPENOBLIVION_TERRAIN_MATERIAL_GENERATOR_HPP
