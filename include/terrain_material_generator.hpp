#ifndef OPENOBLIVION_TERRAIN_MATERIAL_GENERATOR_HPP
#define OPENOBLIVION_TERRAIN_MATERIAL_GENERATOR_HPP

#include "settings.hpp"
#include <OgreMaterialManager.h>
#include <OgreTextureManager.h>
#include <OGRE/Terrain/OgreTerrainMaterialGenerator.h>

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

    const std::string &matName{terrain->getMaterialName()};
    auto matPtr{matMgr.getByName(matName, oo::RESOURCE_GROUP)};
    if (!matPtr) {
      auto baseMat{matMgr.getByName("__LandscapeMaterial", oo::SHADER_GROUP)};
      matPtr = baseMat->clone(terrain->getMaterialName(),
                              true, oo::RESOURCE_GROUP);
    }

    auto *pass{matPtr->getTechnique(0)->getPass(0)};
    if (pass->getNumTextureUnitStates() > 0) return matPtr;

    const std::string &texName{terrain->getLayerTextureName(0, 0)};
    auto *state{pass->createTextureUnitState(texName)};

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
    terrain->_setNormalMapRequired(true);
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
