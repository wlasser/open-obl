#ifndef OPENOBLIVION_OGRE_TERRAIN_MATERIAL_GENERATOR_HPP
#define OPENOBLIVION_OGRE_TERRAIN_MATERIAL_GENERATOR_HPP

#include "settings.hpp"
#include <OgreMaterialManager.h>
#include <OgreTextureManager.h>
#include <Terrain/OgreTerrainMaterialGenerator.h>
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

 private:
  std::pair<Ogre::MaterialPtr, bool>
  createOrRetrieveMaterial(const Ogre::Terrain *terrain);

  std::pair<Ogre::MaterialPtr, bool>
  createOrRetrieveCompositeMaterial(const Ogre::Terrain *terrain);

  void makeHighLodTechnique(const Ogre::Terrain *terrain,
                            Ogre::Technique *technique);
  void makeLowLodTechnique(const Ogre::Terrain *terrain,
                           Ogre::Technique *technique);
  void makeDiffuseCompositeTechnique(const Ogre::Terrain *terrain,
                                     Ogre::Technique *technique);

  /// Create the global normal map for the terrain whose material name is given.
  /// The global normal map's name is dependent on the `Ogre::Terrain` pointer,
  /// which is not available until the terrain is loaded. The `Ogre::Material`
  /// returned by `generate()` is required to reference the global normal map,
  /// but is called during load, and thus we have to create the map in
  /// `generate()` even though it cannot be populated.
  ///
  /// \returns the name of the global normal map texture.
  /// \warning The pixel values of the created texture are unspecified.
  /// \remark If this function is called after the global normal map has already
  ///         been created then nothing happens.
  Ogre::String createGlobalNormalMap(const Ogre::String &matName);

  /// Create the vertex color map for the terrain whose material name is given.
  /// Similarly to the global normal map this needs to be created in
  /// `generate()` but cannot be populated there.
  ///
  /// \returns the name of the vertex color map texture.
  /// \warning The pixel values of the created texture are unspecified.
  /// \remark If this function is called after the vertex color map has already
  ///         been created then nothing happens.
  Ogre::String createVertexColorMap(const Ogre::String &matName);

  Ogre::String createCompositeDiffuseMap(const Ogre::String &matName);
  Ogre::String createCompositeNormalMap(const Ogre::String &matName);
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

#endif // OPENOBLIVION_OGRE_TERRAIN_MATERIAL_GENERATOR_HPP
