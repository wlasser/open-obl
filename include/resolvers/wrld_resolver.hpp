#ifndef OPENOBL_WRLD_RESOLVER_HPP
#define OPENOBL_WRLD_RESOLVER_HPP

#include "atmosphere.hpp"
#include "chrono.hpp"
#include "esp/esp_coordinator.hpp"
#include "job/job.hpp"
#include "math/conversions.hpp"
#include "resolvers/resolvers.hpp"
#include "resolvers/cell_resolver.hpp"
#include "wrld.hpp"
#include <boost/fiber/mutex.hpp>
#include <boost/multi_array.hpp>
#include <OgreSceneManager.h>
#include <Terrain/OgreTerrainGroup.h>
#include <tl/optional.hpp>
#include <mutex>
#include <random>
#include <utility>

namespace oo {

using WrldResolver = Resolver<record::WRLD, oo::BaseId>;

template<>
class Resolver<record::WRLD, oo::BaseId> {
 private:
  struct Metadata {
    /// Accessors, in load order of mods that modify the contents of the world.
    std::vector<oo::EspAccessor> mAccessors{};
    /// All cells in the world.
    std::unordered_set<oo::BaseId> mCells{};
    /// Cells in the world stored in a grid mirroring their actual layout.
    /// The array base is set such that the cell with coordinates `(X,Y)` is
    /// located at `[X][Y]`.
    oo::CellGrid mCellGrid{};

    /// All the reference records in the world and the indices of the cells they
    /// are in.
    std::unordered_map<oo::RefId, oo::CellIndex> mPersistentReferences{};
  };

  /// Holds a record with an immutable backup of the original.
  /// Unlike general record::Record, it is not possible to create a new
  /// record::WRLD at runtime.
  using RecordEntry = std::pair<record::WRLD, tl::optional<record::WRLD>>;
  using WrappedRecordEntry = std::pair<RecordEntry, Metadata>;

  /// Record storage.
  std::unordered_map<oo::BaseId, WrappedRecordEntry> mRecords{};

  /// Record storage mutex.
  mutable boost::fibers::mutex mMtx{};

  class WrldVisitor;
 public:
  using RecordIterator = typename decltype(mRecords)::iterator;

  Resolver() = default;
  ~Resolver() = default;
  Resolver(const Resolver &) = delete;
  Resolver &operator=(const Resolver &) = delete;
  Resolver(Resolver &&) noexcept;
  Resolver &operator=(Resolver &&) noexcept;

  /// Insert a new record with the given accessor if one exists, otherwise
  /// replace the existing record and append the accessor to the accessor list.
  std::pair<RecordIterator, bool>
  insertOrAppend(oo::BaseId baseId,
                 const record::WRLD &rec,
                 oo::EspAccessor accessor);

  /// Return a reference to the world.
  tl::optional<const record::WRLD &> get(oo::BaseId baseId) const;

  /// \overload get(oo::BaseId)
  tl::optional<record::WRLD &> get(oo::BaseId baseId);

  /// Check if there is a world with the baseId.
  bool contains(oo::BaseId baseId) const;

  using BaseResolverContext = std::tuple<Resolver<record::CELL> &>;

  /// Register all cell children of the world.
  void load(oo::BaseId baseId, BaseResolverContext baseCtx);

  /// Return the `BaseId` of the cell at the given position in the given world.
  /// \warning This will return an empty optional if the world has not been
  ///          loaded first with a call to load.
  tl::optional<oo::BaseId>
  getCell(oo::BaseId wrldId, oo::CellIndex index) const;

  /// Return a neighbourhood of the cell at the given position.
  /// Specifically, if \f$d\f$ is the given `diameter`, return the cells with
  /// coordinates \f$(X, Y)\f$ such that \f$(X, Y)\f$ is within the bounds of
  /// the worldspace and
  /// \f[
  ///     \lfloor x - d/2 \rfloor < X \leq \lfloor x + d/2 \rfloor, \quad
  ///     \lfloor y - d/2 \rfloor < Y \leq \lfloor y + d/2 \rfloor.
  /// \f]
  /// `diameter` must be a non-negative integer. If `diameter` is zero and
  /// `cell` is within the bounds of the worldspace, then `cell` is returned.
  /// If the set of cells satisfying the above conditions is empty, the
  /// behaviour is undefined.
  // C++20: Codify the preconditions on cell and diameter.
  oo::CellGridView
  getNeighbourhood(oo::BaseId wrldId, CellIndex cell, int diameter) const;

  /// Return the BaseIds of all cells in the world.
  /// \warning This will return an empty optional if the world has not been
  ///          loaded first with a call to load.
  tl::optional<const std::unordered_set<BaseId> &>
  getCells(oo::BaseId baseId) const;

  /// Return the `BaseId`s of all worldspaces.
  /// This method should generally be avoided but is necessary when trying to
  /// find which worldspace contains a given cell.
  std::unordered_set<BaseId> getWorlds() const;
};

class WrldResolver::WrldVisitor {
 public:
  using Metadata = WrldResolver::Metadata;
  using BaseContext = WrldResolver::BaseResolverContext;

 private:
  Metadata &mMeta;
  BaseContext mBaseCtx;

 public:
  WrldVisitor(Metadata &meta, BaseContext baseCtx)
      : mMeta(meta), mBaseCtx(std::move(baseCtx)) {}

  template<class R> void readRecord(oo::EspAccessor &accessor) {
    (void) mMeta; // Clang bug? Fix -Wunused-private-field
    accessor.skipRecord();
  }
};

// CWG 727
template<> void
WrldResolver::WrldVisitor::readRecord<record::CELL>(oo::EspAccessor &accessor);
// TODO: record::ROAD specialization

class World;

template<> struct ReifyRecordImpl<record::WRLD> {
  using type = std::shared_ptr<World>;
  // WRLD resolver must be nonconst as World may need to load() a parent
  // worldspace.
  using resolvers = decltype(std::tuple_cat(
      std::declval<Resolver<record::WRLD>::BaseResolverContext>(),
      std::declval<std::tuple<oo::Resolver<record::WRLD> &,
                              const oo::Resolver<record::LTEX> &,
                              const oo::Resolver<record::WTHR> &,
                              const oo::Resolver<record::CLMT> &,
                              oo::Resolver<record::LAND> &,
                              const oo::Resolver<record::WATR> &>>()));
  type operator()(const record::WRLD &refRec,
                  Ogre::SceneManager *,
                  btDiscreteDynamicsWorld *,
                  resolvers res,
                  Ogre::SceneNode *);
};

ReifyRecordImpl<record::WRLD>::type
reifyRecord(const record::WRLD &refRec,
            ReifyRecordImpl<record::WRLD>::resolvers res);

class World {
 public:
  using Resolvers = ReifyRecordImpl<record::WRLD>::resolvers;
  using PhysicsWorld = btDiscreteDynamicsWorld;

  gsl::not_null<Ogre::SceneManager *> getSceneManager() const;
  gsl::not_null<PhysicsWorld *> getPhysicsWorld() const;

  oo::BaseId getBaseId() const;
  std::string getName() const;
  void setName(std::string name);

  explicit World(oo::BaseId baseId, std::string name, Resolvers resolvers);

  ~World();
  World(const World &) = delete;
  World &operator=(const World &) = delete;
  World(World &&) = delete;
  World &operator=(World &&) = delete;

  /// Load the terrain of the cell with the given coordinates.
  /// If `async` is true then returns immediately with a `oo::JobCounter` which
  /// will reach zero when the terrain is loaded, otherwise loads the terrain
  /// synchronously and returns nullptr when the terrain is loaded. If `async`
  /// is false then this function must be called on the render thread.
  std::shared_ptr<oo::JobCounter>
  loadTerrain(CellIndex index, bool async = true);

  /// Load the terrain of the given cell, notifying the cell of its terrain.
  void loadTerrain(oo::ExteriorCell &cell);

  /// Load the terrain of the cell with the given id.
  void loadTerrainOnly(oo::BaseId cellId, bool async = true);

  /// Unload the terrain of the given cell, removing its collision object from
  /// the world.
  void unloadTerrain(oo::ExteriorCell &cell);

  /// Unload the terrain of the cell with the given coordinates.
  void unloadTerrain(CellIndex index);

  /// Unload the terrain of the cell with the given id.
  void unloadTerrain(oo::BaseId cellId);

  void updateAtmosphere(const oo::chrono::minutes &time);

 private:
  oo::BaseId mBaseId{};
  std::string mName{};
  std::unique_ptr<Ogre::SceneManager,
                  std::function<void(Ogre::SceneManager *)>> mScnMgr;
  std::unique_ptr<PhysicsWorld> mPhysicsWorld;
  Ogre::TerrainGroup mTerrainGroup;
  Resolvers mResolvers;
  oo::Atmosphere mAtmosphere;

  constexpr static const char
      *CHUNK_BASE_MATERIAL{"__LandscapeMaterialDistant"};

  struct DistantChunk {
    Ogre::SceneNode *node;
    Ogre::MaterialPtr matPtr;
    DistantChunk(Ogre::SceneNode *pNode, Ogre::MaterialPtr pMatPtr) noexcept
        : node(pNode), matPtr(std::move(pMatPtr)) {}
  };
  // C++20: Use autogenerated <=>, hopefully
  struct ChunkIndexComp {
    bool operator()(const ChunkIndex &a, const ChunkIndex &b) const noexcept {
      return qvm::X(a) < qvm::X(b)
          || (qvm::X(a) == qvm::X(b) && qvm::Y(a) < qvm::Y(b));
    }
  };
  using DistantChunkMap = std::map<ChunkIndex, DistantChunk, ChunkIndexComp>;
  DistantChunkMap mDistantChunks{ChunkIndexComp{}};

  constexpr static const char *WATER_MESH_NAME{"__WaterMesh"};
  constexpr static const char *WATER_MANAGER_BASE_NAME{"__WaterManager"};
  constexpr static const char *WATER_BASE_MATERIAL{"__WaterMaterial"};
  constexpr static uint8_t WATER_RENDER_QUEUE_GROUP{
      Ogre::RenderQueueGroupID::RENDER_QUEUE_9
  };
  struct WaterEntry {
    Ogre::SceneNode *node;
    Ogre::InstancedEntity *entity;
    WaterEntry(Ogre::SceneNode *pNode, Ogre::InstancedEntity *pEntity) noexcept
        : node(pNode), entity(pEntity) {}
  };
  // C++20: Use autogenerated <=>, hopefully
  struct CellIndexComp {
    bool operator()(const CellIndex &a, const CellIndex &b) const noexcept {
      return qvm::X(a) < qvm::X(b)
          || (qvm::X(a) == qvm::X(b) && qvm::Y(a) < qvm::Y(b));
    }
  };
  using WaterEntryMap = std::map<CellIndex, WaterEntry, CellIndexComp>;
  WaterEntryMap mWaterPlanes{CellIndexComp{}};

  /// Set up the default `Ogre::Terrain::ImportData` for our
  /// `Ogre::TerrainGroup`.
  /// `Ogre::TerrainGroup` provides a convenient `getDefaultImportSettings()`
  /// method to obtain an `Ogre::Terrain::ImportData` with customizable
  /// defaults, which should be preferred to default-constructing an
  /// `Ogre::Terrain::ImportData` and populating it manually.
  void setDefaultImportData();

  tl::optional<oo::BaseId> getLandId(oo::BaseId cellId);
  tl::optional<oo::BaseId> getLandId(oo::BaseId cellId, oo::BaseId wrldId);

  tl::optional<oo::BaseId> getWatrId();
  tl::optional<oo::BaseId> getWatrId(oo::BaseId wrldId);

  oo::BaseId getAncestorWrldId();
  oo::BaseId getAncestorWrldId(oo::BaseId wrldId);

  void loadWaterPlane(CellIndex index, const record::CELL &cellRec);
  void unloadWaterPlane(CellIndex index);

  DistantChunk makeChunk(oo::ChunkIndex chunkIndex);
  void makeDistantCellGrid();

  void makeCellGrid();
  void makePhysicsWorld();

  using ImportDataArray = std::array<Ogre::Terrain::ImportData, 4u>;

  void setTerrainHeights(const record::raw::VHGT &rec,
                         ImportDataArray &importData) const;

  /// Opacity of the layer at each point in a quadrant.
  using QuadrantBlendMap = std::array<uint8_t,
                                      oo::verticesPerQuad<std::size_t>
                                          * oo::verticesPerQuad<std::size_t>>;

  /// Ordering of layers in a quadrant or in a cell, depending on context.
  using LayerOrder = std::vector<oo::BaseId>;

  /// Map taking each LTEX id to a blend map, for a fixed quadrant.
  /// \remark Access via [] will value-initialize any id which doesn't exist,
  ///         giving a transparent QuadrantBlendMap.
  using LayerMap = std::unordered_map<oo::BaseId, QuadrantBlendMap>;

  void emplaceTexture(Ogre::StringVector &list, std::string texName) const;

  using LayerMaps = std::array<LayerMap, 4u>;
  using LayerOrders = std::array<LayerOrder, 4u>;

  LayerMaps makeDefaultLayerMaps() const;
  LayerOrders makeDefaultLayerOrders() const;

  void applyBaseLayers(LayerMaps &layerMaps, const record::LAND &rec) const;
  void applyBaseLayers(LayerOrders &layerOrders, const record::LAND &rec) const;

  void applyFineLayers(LayerMaps &layerMaps, const record::LAND &rec) const;
  void applyFineLayers(LayerOrders &layerOrders, const record::LAND &rec) const;
};

} // namespace oo

#endif // OPENOBL_WRLD_RESOLVER_HPP
