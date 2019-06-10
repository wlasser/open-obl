#ifndef OPENOBL_WRLD_IMPL_HPP
#define OPENOBL_WRLD_IMPL_HPP

#include "atmosphere.hpp"
#include "job/job.hpp"
#include "math/conversions.hpp"
#include "resolvers/wrld_resolver.hpp"
#include <OgrePrerequisites.h>
#include <Terrain/OgreTerrainGroup.h>

namespace oo {

// C++20: Use autogenerated <=>, hopefully
template<class T> struct IndexComp {
  bool operator()(const T &a, const T &b) const noexcept {
    return qvm::X(a) < qvm::X(b)
        || (qvm::X(a) == qvm::X(b) && qvm::Y(a) < qvm::Y(b));
  }
};

struct DistantChunk {
  Ogre::SceneNode *node;
  Ogre::MaterialPtr matPtr;
  DistantChunk(Ogre::SceneNode *pNode, Ogre::MaterialPtr pMatPtr) noexcept
      : node(pNode), matPtr(std::move(pMatPtr)) {}
};

struct WaterEntry {
  Ogre::SceneNode *node;
  Ogre::InstancedEntity *entity;
  WaterEntry(Ogre::SceneNode *pNode, Ogre::InstancedEntity *pEntity) noexcept
      : node(pNode), entity(pEntity) {}
};

/// Alpha value of a texture layer at each point in a quadrant.
/// Vertices in the quadrant are laid out in row-major order so that the point
/// with local coordinates `(x, y)` is at index `vpq * y + x` where
/// `vpq = oo::verticesPerQuad<std::size_t>`.
using QuadrantBlendMap = std::array<uint8_t,
                                    oo::verticesPerQuad<std::size_t>
                                        * oo::verticesPerQuad<std::size_t>>;

/// Ordering of layers in a quadrant or in a cell, depending on the context.
/// Each `oo::BaseId` refers to the BaseId of the `record::LTEX` describing the
/// texture layer.
using LayerOrder = std::vector<oo::BaseId>;

/// Map assigning a blend map to `record::LTEX`s describing texture layers, for
/// a fixed quadrant.
/// \remark Access via `[]` will value-initialize any id which doesn't already
///         exist, giving a transparent `QuadrantBlendMap` for that layer.
using LayerMap = std::unordered_map<oo::BaseId, QuadrantBlendMap>;

/// `oo::LayerMap`s for each quadrant of a cell.
using LayerMaps = std::array<LayerMap, 4u>;

/// `oo::LayerOrder`s for each quadrant of a cell.
using LayerOrders = std::array<LayerOrder, 4u>;

/// `Ogre::Terrain::ImportData` for each quadrant of a cell.
using ImportDataArray = std::array<Ogre::Terrain::ImportData, 4u>;

/// Copy the terrain normals in the `record::VNML` of a `record::LAND` into a
/// pixel box representing a cell. If the `record::LAND` has no normals, then
/// vertical normals are copied into the pixel box instead.
void writeNormals(Ogre::PixelBox dst, const record::LAND &rec);

/// Copy the vertex colours in the `record::VCLR` of a `record::LAND` into a
/// pixel box representing a cell. If the `record::LAND` has no vertex colours,
/// then white vertex colours are copied into the pixel box instead.
void writeVertexCols(Ogre::PixelBox dst, const record::LAND &rec);

/// Construct a set of `oo::LayerMaps` for a cell, giving each quadrant a single
/// opaque layer described by an imaginary `record::LTEX` with BaseId `0`.
LayerMaps makeDefaultLayerMaps();

/// Construct a set of `oo::LayerOrders` for a cell, giving each quadrant a
/// single layer described by an imaginary `record::LTEX` with BaseId `0`.
LayerOrders makeDefaultLayerOrders();

/// Construct an opaque layer in the appropriate quadrant for each
/// `record::BTXT` base texture in the `record::LAND`, overwriting any existing
/// layers.
/// In particular, the layer described by the imaginary `record::LTEX` with
/// BaseId `0` created by `oo::makeDefaultLayerMaps()` is replaced in any
/// quadrant where a `record::BTXT` is found.
void applyBaseLayers(LayerMaps &layerMaps, const record::LAND &rec);

/// Insert a base layer at the start of the layer order for the appropriate
/// quadrant, for each `record::BTXT` base texture in the `record::LAND`.
/// Any existing layer at the front of the order is replaced, including in
/// particular the imaginary `record::LTEX` with BaseId `0` created by
/// `oo::makeDefaultLayerOrders()`.
void applyBaseLayers(LayerOrders &layerOrders, const record::LAND &rec);

/// Insert a fine texture layer in the appropriate quadrant for every
/// `record::ATXT` / `record::VTXT` pair in the `record::LAND`.
void applyFineLayers(LayerMaps &layerMaps, const record::LAND &rec);

/// Insert each fine texture layer described by a
/// `record::ATXT` / `record::VTXT` pair in the `record::LAND` into the
/// `oo::LayerOrder` for the appropriate quadrant, in the order that they appear
/// in the `record::LAND`.
void applyFineLayers(LayerOrders &layerOrders, const record::LAND &rec);

/// Copy the given texture layers into the layer blend maps of the given
/// terrain quadrant.
/// \remark Must be run on the render fiber.
/// \remark `layerMap` is taken by nonconst ref so `[]` can be used to create
///         an empty quadrant blend map if one doesn't exist.
void applyLayerMap(Ogre::Terrain *quad,
                   LayerMap &layerMap,
                   const LayerOrder &layerOrder);

/// Copy the vertex normals from the subvolume `region` of the `src` into the
/// global normal map texture of the terrain material `matName`.
/// \remark Must be run on the render fiber.
/// \see oo::TerrainMaterialGenerator
void blitNormals(std::string matName, Ogre::PixelBox src, Ogre::Box region);

/// Copy the vertex colours from the subvolume `region` of the `src` into the
/// global vertex colour texture of the terrain material `matName`.
/// \remark Must be run on the render fiber.
/// \see oo::TerrainMaterialGenerator
void blitVertexCols(std::string matName, Ogre::PixelBox src, Ogre::Box region);

/// Copy the vertex normals, vertex colours, and texture layers onto the given
/// terrain quadrant.
void blitTerrainTextures(Ogre::Terrain *quad,
                         LayerMap &layerMap,
                         const LayerOrder &layerOrder,
                         Ogre::PixelBox normals,
                         Ogre::PixelBox vertexColors,
                         Ogre::Box region);

/// Append the landscape texture name and its normal map to the list of texture
/// names.
/// Specifically, append a string equal to `"textures/landscape/" + texName` to
/// the back of `list`. If that texture has a normal map then append that
/// normal map too, otherwise append the flat normal map.
void emplaceTerrainTexture(Ogre::StringVector &list, std::string texName);

/// Copy the height data from the `record::VHGT` into the height data of the
/// `importData` defining the terrain of a cell.
void
setTerrainHeights(ImportDataArray &importData, const record::raw::VHGT &rec);

class World::WorldImpl {
 public:
  using Resolvers = World::Resolvers;
  using PhysicsWorld = World::PhysicsWorld;

  explicit WorldImpl(oo::BaseId baseId, std::string name, Resolvers resolvers);
  ~WorldImpl();

  gsl::not_null<Ogre::SceneManager *> getSceneManager() const;
  gsl::not_null<PhysicsWorld *> getPhysicsWorld() const;

  oo::BaseId getBaseId() const;
  std::string getName() const;
  void setName(std::string name);

  /// Load the OGRE terrain at the given coordinates.
  /// If `async` is true then this returns immediately with an `oo::JobCounter`
  /// which will reach zero when the terrain is loaded, otherwise the terrain is
  /// loaded synchronously and this function returns `nullptr` when the loading
  /// is complete.
  std::shared_ptr<oo::JobCounter>
  loadTerrain(CellIndex index, bool async = true);

  /// Unload the OGRE terrain at the given coordinates.
  void unloadTerrain(CellIndex index);

  void loadTerrain(oo::ExteriorCell &cell);
  void unloadTerrain(oo::ExteriorCell &cell);

  void loadTerrain(oo::BaseId cellId, bool async = true);
  void unloadTerrain(oo::BaseId cellId);

  void updateAtmosphere(const oo::chrono::minutes &time);

 private:
  constexpr static const char *CHUNK_BASE_MATERIAL{
      "__LandscapeMaterialDistant"
  };
  constexpr static const char *WATER_MESH_NAME{"__WaterMesh"};
  constexpr static const char *WATER_MANAGER_BASE_NAME{"__WaterManager"};
  constexpr static const char *WATER_BASE_MATERIAL{"__WaterMaterial"};
  constexpr static uint8_t WATER_RENDER_QUEUE_GROUP{
      Ogre::RenderQueueGroupID::RENDER_QUEUE_9
  };

  using SceneManagerDeleter = std::function<void(Ogre::SceneManager *)>;

  using ChunkIndexCmp = IndexComp<ChunkIndex>;
  using CellIndexCmp = IndexComp<CellIndex>;

  using DistantChunkMap = std::map<ChunkIndex, DistantChunk, ChunkIndexCmp>;
  using WaterEntryMap = std::map<CellIndex, WaterEntry, CellIndexCmp>;

  tl::optional<const record::CELL &> getCell(oo::BaseId cellId) const;

  std::shared_ptr<oo::JobCounter> loadTerrainAsyncImpl(CellIndex index);
  std::shared_ptr<oo::JobCounter> loadTerrainSyncImpl(CellIndex index);

  std::array<Ogre::Terrain *, 4u> getTerrainQuads(CellIndex index) const;

  // \pre Called on render thread
  bool isTerrainLoaded(CellIndex index) const noexcept;

  /// Set up the default `Ogre::Terrain::ImportData` for our
  /// `Ogre::TerrainGroup`.
  /// `Ogre::TerrainGroup` provides a convenient `getDefaultImportSettings()`
  /// method to obtain an `Ogre::Terrain::ImportData` with customizable
  /// defaults, which should be preferred to default-constructing an
  /// `Ogre::Terrain::ImportData` and populating it manually.
  void setDefaultImportData();

  tl::optional<oo::BaseId> getLandId(oo::BaseId cellId) const noexcept;
  tl::optional<oo::BaseId> getLandId(oo::BaseId cellId,
                                     oo::BaseId wrldId) const noexcept;

  tl::optional<oo::BaseId> getWatrId() const noexcept;
  tl::optional<oo::BaseId> getWatrId(oo::BaseId wrldId) const noexcept;

  oo::BaseId getAncestorWrldId() const noexcept;
  oo::BaseId getAncestorWrldId(oo::BaseId wrldId) const noexcept;

  void makeWaterPlane() const;
  Ogre::MaterialPtr makeWaterMaterial() const;
  void makeWaterInstanceManager() const;

  void loadWaterPlane(CellIndex index, const record::CELL &cellRec);
  void unloadWaterPlane(CellIndex index);

  DistantChunk makeChunk(oo::ChunkIndex chunkIndex);
  void makeDistantCellGrid();

  void makeCellGrid();
  void makePhysicsWorld();

  oo::BaseId mBaseId{};
  std::string mName{};
  Resolvers mResolvers;
  std::unique_ptr<Ogre::SceneManager, SceneManagerDeleter> mScnMgr;
  std::unique_ptr<PhysicsWorld> mPhysicsWorld;
  Ogre::TerrainGroup mTerrainGroup;
  oo::Atmosphere mAtmosphere;
  DistantChunkMap mDistantChunks{ChunkIndexCmp{}};
  WaterEntryMap mWaterPlanes{CellIndexCmp{}};
};

} // namespace oo

#endif // OPENOBL_WRLD_IMPL_HPP