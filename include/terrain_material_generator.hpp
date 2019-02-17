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

  Ogre::MaterialPtr generate(const Ogre::Terrain *terrain) override;

  Ogre::MaterialPtr
  generateForCompositeMap(const Ogre::Terrain *terrain) override;

  uint8_t getMaxLayers(const Ogre::Terrain *terrain) const override;

  bool isVertexCompressionSupported() const override;

  void requestOptions(Ogre::Terrain *terrain) override;

  void setLightmapEnabled(bool enabled) override;

  void updateParams(const Ogre::MaterialPtr &mat,
                    const Ogre::Terrain *terrain) override;

  void updateParamsForCompositeMap(const Ogre::MaterialPtr &mat,
                                   const Ogre::Terrain *terrain) override;
};

class TerrainMaterialGenerator : public Ogre::TerrainMaterialGenerator {
 public:
  TerrainMaterialGenerator();
  ~TerrainMaterialGenerator() override = default;

  const Ogre::TerrainLayerDeclaration &getLayerDeclaration() const override;
 private:
  Ogre::TerrainLayerDeclaration mDecl;
};

} // namespace oo

#endif // OPENOBLIVION_TERRAIN_MATERIAL_GENERATOR_HPP
