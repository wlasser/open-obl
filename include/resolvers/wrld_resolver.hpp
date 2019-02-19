#ifndef OPENOBLIVION_WRLD_RESOLVER_HPP
#define OPENOBLIVION_WRLD_RESOLVER_HPP

#include "conversions.hpp"
#include "esp_coordinator.hpp"
#include "resolvers/resolvers.hpp"
#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>
#include <boost/multi_array.hpp>
#include <OgreRoot.h>
#include <OgreSceneManager.h>
#include <OGRE/Terrain/OgreTerrainGroup.h>
#include <tl/optional.hpp>
#include <utility>

namespace oo {

template<>
class Resolver<record::WRLD> {
 private:
  struct Metadata {
    /// Accessors, in load order of mods that modify the contents of the world.
    std::vector<oo::EspAccessor> mAccessors{};
    /// All cells in the world.
    absl::flat_hash_set<oo::BaseId> mCells{};
  };

  /// Holds a record with an immutable backup of the original.
  /// Unlike general record::Record, it is not possible to create a new
  /// record::WRLD at runtime.
  using RecordEntry = std::pair<record::WRLD, tl::optional<record::WRLD>>;
  using WrappedRecordEntry = std::pair<RecordEntry, Metadata>;

  /// Record storage.
  absl::flat_hash_map<oo::BaseId, WrappedRecordEntry> mRecords{};
  class WrldVisitor;
 public:
  using RecordIterator = typename decltype(mRecords)::iterator;

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
                              oo::Resolver<record::LAND> &>>()));
};

/// Not a specialization because passing an Ogre::SceneManager doesn't make
/// sense.
ReifyRecordTrait<record::WRLD>::type
reifyRecord(const record::WRLD &refRec,
            ReifyRecordTrait<record::WRLD>::resolvers resolvers);

class World {
 public:
  using CellIndex = qvm::vec<int32_t, 2>;
  using Resolvers = ReifyRecordTrait<record::WRLD>::resolvers;
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

  /// Get the coordinates of the cell containing the given position.
  /// \remark `x` and `y` are measured in units.
  CellIndex getCellIndex(float x, float y) const;

  /// Get the oo::BaseId of the cell with the given coordinates.
  oo::BaseId getCell(CellIndex index) const;

  /// Load the terrain of the cell with the given coordinates.
  void loadTerrain(CellIndex index);

  /// Load the terrain of the given cell, notifying the cell of its terrain.
  void loadTerrain(oo::ExteriorCell &cell);

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
  auto getNeighbourhood(CellIndex cell, int diameter) const {
    const int x{qvm::X(cell)}, y{qvm::Y(cell)};
    // Adding 1 maps intervals (a, b] -> [a, b)

    // For integer i and real r,
    // floor(i - r) = i - ceil(r)
    const int x0{1 + x - diameter / 2 - (diameter % 2 == 0 ? 0 : 1)};
    const int y0{1 + y - diameter / 2 - (diameter % 2 == 0 ? 0 : 1)};
    // floor(i + r) = i + floor(r)
    const int x1{1 + x + diameter / 2};
    const int y1{1 + y + diameter / 2};

    // Keep within bounds.
    const int X0{std::max<int>(x0, mCells.index_bases()[0])};
    const int Y0{std::max<int>(y0, mCells.index_bases()[1])};
    const int
        X1{std::min<int>(x1, mCells.index_bases()[0] + mCells.shape()[0])};
    const int
        Y1{std::min<int>(y1, mCells.index_bases()[1] + mCells.shape()[1])};

    using Range = CellGrid::index_range;
    return mCells[boost::indices[Range(X0, X1)][Range(Y0, Y1)]];
  }

 private:
  oo::BaseId mBaseId{};
  std::string mName{};
  gsl::not_null<gsl::owner<Ogre::SceneManager *>> mScnMgr;
  std::unique_ptr<PhysicsWorld> mPhysicsWorld;
  Ogre::TerrainGroup mTerrainGroup;
  Resolvers mResolvers;

  using CellGrid = boost::multi_array<oo::BaseId, 2>;

  /// Cells in the world stored in a grid mirroring their actual layout.
  /// The array base is set such that the cell with coordinates `(X,Y)` is
  /// located at `[X][Y]`.
  CellGrid mCells{};

  void makeCellGrid();
  void makePhysicsWorld();
  void makeAtmosphere();

  void setTerrainHeights(const record::raw::VHGT &rec,
                         Ogre::Terrain::ImportData &importData) const;

  /// Opacity of the layer at each point in a quadrant.
  using QuadrantBlendMap = std::array<uint8_t, 17u * 17u>;

  /// Ordering of layers in a quadrant.
  using LayerOrder = std::vector<oo::BaseId>;

  /// Map taking each LTEX id to a blend map, for a fixed quadrant.
  /// \remark Access via [] will value-initialize any id which doesn't exist,
  ///         giving a transparent QuadrantBlendMap.
  using LayerMap = absl::flat_hash_map<oo::BaseId, QuadrantBlendMap>;

  void emplaceTexture(Ogre::StringVector &list, std::string texName) const;

  std::array<LayerMap, 4u>
  makeDefaultLayerMaps(const record::LAND &rec) const;
  std::array<LayerOrder, 4u>
  makeDefaultLayerOrders(const record::LAND &rec) const;
};

} // namespace oo

#endif // OPENOBLIVION_WRLD_RESOLVER_HPP
