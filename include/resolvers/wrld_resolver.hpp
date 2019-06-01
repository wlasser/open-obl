#ifndef OPENOBL_WRLD_RESOLVER_HPP
#define OPENOBL_WRLD_RESOLVER_HPP

#include "chrono.hpp"
#include "esp/esp_coordinator.hpp"
#include "resolvers/resolvers.hpp"
#include "resolvers/cell_resolver.hpp"
#include "wrld.hpp"
#include <boost/fiber/mutex.hpp>
#include <OgrePrerequisites.h>

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

  /// Load the terrain of the given cell, notifying the cell of its terrain, and
  /// adding its collision object to the world.
  void loadTerrain(oo::ExteriorCell &cell);

  /// Unload the terrain of the given cell, removing its collision object from
  /// the world.
  void unloadTerrain(oo::ExteriorCell &cell);

  /// Load the terrain of the cell with the given id.
  void loadTerrainOnly(oo::BaseId cellId, bool async = true);

  /// Unload the terrain of the cell with the given id.
  void unloadTerrain(oo::BaseId cellId);

  void updateAtmosphere(const oo::chrono::minutes &time);

 private:
  class WorldImpl;
  std::unique_ptr<WorldImpl> mImpl;
};

} // namespace oo

#endif // OPENOBL_WRLD_RESOLVER_HPP
