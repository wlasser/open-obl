#ifndef OPENOBLIVION_WRLD_RESOLVER_HPP
#define OPENOBLIVION_WRLD_RESOLVER_HPP

#include "conversions.hpp"
#include "esp_coordinator.hpp"
#include "job/job.hpp"
#include "resolvers/resolvers.hpp"
#include "resolvers/cell_resolver.hpp"
#include "resolvers/wthr_resolver.hpp"
#include "time_manager.hpp"
#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>
#include <boost/fiber/mutex.hpp>
#include <boost/multi_array.hpp>
#include <OgreRoot.h>
#include <OgreSceneManager.h>
#include <OGRE/Terrain/OgreTerrainGroup.h>
#include <tl/optional.hpp>
#include <mutex>
#include <random>
#include <utility>

namespace oo {

/// Coordinates of an `oo::ExteriorCell` in a `World`.
using CellIndex = qvm::vec<int32_t, 2>;

/// Get the coordinates of the `oo::ExteriorCell` containing the given position.
/// \remark `x` and `y` are measured in BS units.
CellIndex getCellIndex(float x, float y) noexcept;

template<>
class Resolver<record::WRLD> {
 private:
  struct Metadata {
    /// Accessors, in load order of mods that modify the contents of the world.
    std::vector<oo::EspAccessor> mAccessors{};
    /// All cells in the world.
    absl::flat_hash_set<oo::BaseId> mCells{};
    /// All the reference records in the world and the indices of the cells they
    /// are in.
    absl::flat_hash_map<oo::RefId, oo::CellIndex> mPersistentReferences{};
  };

  /// Holds a record with an immutable backup of the original.
  /// Unlike general record::Record, it is not possible to create a new
  /// record::WRLD at runtime.
  using RecordEntry = std::pair<record::WRLD, tl::optional<record::WRLD>>;
  using WrappedRecordEntry = std::pair<RecordEntry, Metadata>;

  /// Record storage.
  absl::flat_hash_map<oo::BaseId, WrappedRecordEntry> mRecords{};

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

  /// Return the BaseIds of all cells in the world.
  /// \warning This will return an empty optional if the world has not been
  ///          loaded first with a call to load.
  tl::optional<const absl::flat_hash_set<BaseId> &>
  getCells(oo::BaseId baseId) const;

  /// Return the `BaseId`s of all worldspaces.
  /// This method should generally be avoided but is necessary when trying to
  /// find which worldspace contains a given cell.
  absl::flat_hash_set<BaseId> getWorlds() const;
};

class Resolver<record::WRLD>::WrldVisitor {
 public:
  using Metadata = Resolver<record::WRLD>::Metadata;
  using BaseContext = Resolver<record::WRLD>::BaseResolverContext;

 private:
  Metadata &mMeta;
  BaseContext mBaseCtx;

 public:
  WrldVisitor(Metadata &meta, BaseContext baseCtx)
      : mMeta(meta), mBaseCtx(std::move(baseCtx)) {}

  template<class R> void readRecord(oo::EspAccessor &accessor) {
    accessor.skipRecord();
  }

  template<> void readRecord<record::CELL>(oo::EspAccessor &accessor);
  // TODO: record::ROAD specialization
};

class World;

template<>
struct ReifyRecordTrait<record::WRLD> {
  using type = std::shared_ptr<World>;
  using resolvers = decltype(std::tuple_cat(
      std::declval<Resolver<record::WRLD>::BaseResolverContext>(),
      std::declval<std::tuple<const oo::Resolver<record::WRLD> &,
                              const oo::Resolver<record::LTEX> &,
                              const oo::Resolver<record::WTHR> &,
                              const oo::Resolver<record::CLMT> &,
                              oo::Resolver<record::LAND> &>>()));
};

/// Not a specialization because passing an Ogre::SceneManager doesn't make
/// sense.
ReifyRecordTrait<record::WRLD>::type
reifyRecord(const record::WRLD &refRec,
            ReifyRecordTrait<record::WRLD>::resolvers resolvers);

class World {
 public:
  using Resolvers = ReifyRecordTrait<record::WRLD>::resolvers;
  using PhysicsWorld = btDiscreteDynamicsWorld;
  using CellGridView = boost::multi_array<oo::BaseId,
                                          2>::const_array_view<2>::type;

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

  /// Get the oo::BaseId of the cell with the given coordinates.
  oo::BaseId getCell(CellIndex index) const;

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
  CellGridView getNeighbourhood(CellIndex cell, int diameter) const;

 private:
  oo::BaseId mBaseId{};
  std::string mName{};
  gsl::not_null<gsl::owner<Ogre::SceneManager *>> mScnMgr;
  std::unique_ptr<PhysicsWorld> mPhysicsWorld;
  Ogre::TerrainGroup mTerrainGroup;
  Resolvers mResolvers;

  oo::chrono::minutes mSunriseBegin{};
  oo::chrono::minutes mSunriseEnd{};
  oo::chrono::minutes mSunsetBegin{};
  oo::chrono::minutes mSunsetEnd{};
  bool mHasMasser{false};
  bool mHasSecunda{false};
  bool mHasSun{false};
  unsigned int mPhaseLength{0};
  std::vector<oo::Weather> mWeathers{};
  std::discrete_distribution<> mWeatherDistribution{};
  float mVolatility{};
  std::size_t mCurrentWeather{0};

  using CellGrid = boost::multi_array<oo::BaseId, 2>;

  /// Cells in the world stored in a grid mirroring their actual layout.
  /// The array base is set such that the cell with coordinates `(X,Y)` is
  /// located at `[X][Y]`.
  CellGrid mCells{};

  /// Set up the default `Ogre::Terrain::ImportData` for our
  /// `Ogre::TerrainGroup`.
  /// `Ogre::TerrainGroup` provides a convenient `getDefaultImportSettings()`
  /// method to obtain an `Ogre::Terrain::ImportData` with customizable
  /// defaults, which should be preferred to default-constructing an
  /// `Ogre::Terrain::ImportData` and populating it manually.
  void setDefaultImportData();

  void makeCellGrid();
  void makePhysicsWorld();
  void makeAtmosphere();

  void makeClimateSettings(const record::CLMT &rec);
  void makeWeatherList(const record::CLMT &rec);
  void makeSun(const record::CLMT &rec);

  constexpr static const char *SUN_NODE{"__sunNode"};
  constexpr static const char *SUN_LIGHT{"__sunLight"};
  constexpr static const char *SUN_BILLBOARD_SET{"__sunBillboardSet"};
  constexpr static const char *SUN_BASE_MATERIAL{"__sunMaterial"};

  /// Return the sun scene node, possibly creating it, along with a boolean
  /// indicating whether it was created or not.
  std::pair<Ogre::SceneNode *, bool> createOrRetrieveSunNode();
  /// Return the sun light, possibly creating it, along with a boolean
  /// indicating whether it was created or not.
  std::pair<Ogre::Light *, bool> createOrRetrieveSunLight();
  /// Return the sun billboard set, possibly creating it and the sun billboard,
  /// along with a boolean indicating whether it was created or not.
  std::pair<Ogre::BillboardSet *, bool> createOrRetrieveSunBillboardSet();
  /// Return the sun material for the given climate, possibly creating it,
  /// along with a boolean indicating whether it was created or not.
  std::pair<Ogre::MaterialPtr, bool>
  createOrRetrieveSunMaterial(const record::CLMT &rec);

  /// Return the sun scene node, or nullptr if it doesn't exist
  Ogre::SceneNode *getSunNode() const noexcept;
  /// Return the sun light, or nullptr if it doesn't exist
  Ogre::Light *getSunLight() const noexcept;
  /// Return the sun billboard, or nullptr if it doesn't exist
  Ogre::Billboard *getSunBillboard() const noexcept;

  /// Get the position of the sun at the given time of day, relative to an
  /// observer.
  Ogre::Vector3 getSunPosition(const chrono::minutes &time) const;

  /// 'Simple' implementation of `getSunPosition()`.
  /// This function assumes that the sun moves at a uniform velocity in a
  /// semicircular arc through the zenith, rising above the east horizon at the
  /// beginning of sunrise and setting below the west horizon at the end of
  /// sunset.
  Ogre::Vector3 getSunPositionSimple(const chrono::minutes &time) const;

  /// 'Physical' implementation of `getSunPosition()`.
  /// The position of the sun on the celestial sphere is calculated based on the
  /// time of year. This position is converted into an apparent position as
  /// seen by an observer at a fixed longitude and latitude at the given time
  /// of day.
  ///
  /// Obviously this assumes that the solar system in-game behaves in a similar
  /// manner to the real-world. Since a lot of the necessary astronomical
  /// information is---justifiably, since why would anybody care---missing from
  /// the game, we have to take some liberties and assume that Nirn is like
  /// Earth in a lot of ways. In particular, we assume the same obliquity of
  /// the ecliptic and roughly the same orbital shape. These aren't necessary
  /// assumptions, one could take whatever values they like, but copying Earth
  /// keeps things looking realistic.
  ///
  /// Because we don't know the size of Nirn, we don't try to update the
  /// observer's (geographic) latitude and longitude based on their position in
  /// the game world. Instead, we just put them in the northern hemisphere along
  /// the prime meridian.
  ///
  /// \todo Use the actual time of year, instead of the epoch.
  Ogre::Vector3 getSunPositionPhysical(const chrono::minutes &time) const;

  struct EquatorialCoordinates {
    Ogre::Radian rightAscension{};
    Ogre::Radian declination{};
  };

  /// Return the position of the sun in equatorial coordinates based on the time
  /// of year. This is an implementation function for `getSunPositionPhysical`.
  EquatorialCoordinates getSunEquatorialCoordinates() const;

  /// Set the sunrise and sunset times based on the given sun coordinates and
  /// observer latitude.
  void setSunriseSunsetTimes(const Ogre::Radian &declination,
                             const Ogre::Radian &latitude);

  /// Split a time in minutes from 12:00 am into a `(time of day, t)` pair
  /// required by `oo::Weather`.
  std::pair<chrono::QualitativeTimeOfDay, float>
  splitTime(const chrono::minutes &time) const noexcept;

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
  using LayerMap = absl::flat_hash_map<oo::BaseId, QuadrantBlendMap>;

  void emplaceTexture(Ogre::StringVector &list, std::string texName) const;

  using LayerMaps = std::array<LayerMap, 4u>;
  using LayerOrders = std::array<LayerOrder, 4u>;

  LayerMaps makeDefaultLayerMaps(const record::LAND &rec) const;
  LayerOrders makeDefaultLayerOrders(const record::LAND &rec) const;

  void applyBaseLayers(LayerMaps &layerMaps, const record::LAND &rec) const;
  void applyBaseLayers(LayerOrders &layerOrders, const record::LAND &rec) const;

  void applyFineLayers(LayerMaps &layerMaps, const record::LAND &rec) const;
  void applyFineLayers(LayerOrders &layerOrders, const record::LAND &rec) const;
};

} // namespace oo

#endif // OPENOBLIVION_WRLD_RESOLVER_HPP
