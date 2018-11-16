#ifndef OPENOBLIVION_CELL_RESOLVER_HPP
#define OPENOBLIVION_CELL_RESOLVER_HPP

#include "bullet/configuration.hpp"
#include "esp_coordinator.hpp"
#include "resolvers/door_resolver.hpp"
#include "resolvers/helpers.hpp"
#include "resolvers/light_resolver.hpp"
#include "resolvers/resolvers.hpp"
#include "resolvers/static_resolver.hpp"
#include <btBulletDynamicsCommon.h>
#include <tl/optional.hpp>
#include <OgreRoot.h>
#include <OgreSceneManager.h>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

using CellResolver = Resolver<record::CELL>;

template<>
class Resolver<record::CELL> {
 private:
  struct Metadata {
    /// Time that the player most recently left the cell, in in-game hours.
    /// This is measured from the epoch.
    int mDetachTime{};
    /// Accessors, in load order of mods that modify the contents of the cell.
    std::vector<esp::EspAccessor> mAccessors{};
    /// All reference records inside the cell.
    /// This includes both esp records and ess records.
    std::unordered_set<RefId> mReferences{};
  };

  /// Holds a record with an immutable backup of the original.
  /// Unlike general record::Record, it is not possible to create a new
  /// record::CELL at runtime.
  using RecordEntry = std::pair<record::CELL, tl::optional<record::CELL>>;
  using WrappedRecordEntry = std::pair<RecordEntry, Metadata>;

  /// Record storage.
  std::unordered_map<BaseId, WrappedRecordEntry> mRecords{};

  /// Bullet configuration, for constructing physics worlds.
  const bullet::Configuration &mBulletConf;

  using RecordIterator = typename decltype(mRecords)::iterator;

  /// Insert a new record with the given accessor if one exists, otherwise
  /// replace the existing record and append the accessor to the accessor list.
  std::pair<RecordIterator, bool>
  insertOrAppend(BaseId baseId,
                 const record::CELL &rec,
                 esp::EspAccessor accessor);

  friend class InitialRecordVisitor;
  class CellVisitor;
 public:

  /// The bullet::Configuration is necessary to construct cells.
  /// bulletConf should live for at least as long as this object.
  explicit Resolver<record::CELL>(const bullet::Configuration &bulletConf)
      : mBulletConf(bulletConf) {}

  /// Get the underlying bullet configuration used for creating physics worlds.
  const bullet::Configuration &getBulletConfiguration() const;

  /// Return a reference to the cell.
  tl::optional<const record::CELL &> get(BaseId baseId) const;

  /// \overload get(BaseId)
  tl::optional<record::CELL &> get(BaseId baseId);

  /// Reset the detach time for a cell to the given time, in in-game hours, from
  /// the epoch.
  void setDetachTime(BaseId baseId, int detachTime);

  /// Return the detach time for the given cell in in-game hours from the epoch.
  int getDetachTime(BaseId baseId) const;

  /// Checks if there is a cell with the baseId
  bool contains(BaseId baseId) const;

  using RefrResolverContext = std::tuple<Resolver<record::REFR_STAT, RefId> &,
                                         Resolver<record::REFR_DOOR, RefId> &,
                                         Resolver<record::REFR_LIGH, RefId> &>;
  using BaseResolverContext = std::tuple<const Resolver<record::STAT> &,
                                         const Resolver<record::DOOR> &,
                                         const Resolver<record::LIGH> &>;

  /// Load all child references of a cell.
  void load(BaseId baseId,
            RefrResolverContext refrCtx,
            BaseResolverContext baseCtx);

  /// Return the RefIds of all reference records in the cell.
  /// \warning This will return an empty std::vector if the cell has not been
  ///          loaded first with a call to load.
  tl::optional<const std::unordered_set<RefId> &>
  getReferences(BaseId baseId) const;
};

class Resolver<record::CELL>::CellVisitor {
 public:
  using Metadata = Resolver<record::CELL>::Metadata;
  using RefrContext = Resolver<record::CELL>::RefrResolverContext;
  using BaseContext = Resolver<record::CELL>::BaseResolverContext;
 private:
  Metadata &mMeta;
  RefrContext mRefrCtx;
  BaseContext mBaseCtx;

 public:
  CellVisitor(Metadata &meta, RefrContext refrCtx, BaseContext baseCtx)
      : mMeta(meta),
        mRefrCtx(std::move(refrCtx)),
        mBaseCtx(std::move(baseCtx)) {}

  template<class R>
  void readRecord(esp::EspAccessor &accessor) {
    accessor.skipRecord();
  }

  template<>
  void readRecord<record::REFR>(esp::EspAccessor &accessor);
};

class Cell {
 public:
  using PhysicsWorld = btDiscreteDynamicsWorld;
  std::string name{};
  BaseId baseId{};
  gsl::not_null<gsl::owner<Ogre::SceneManager *>> scnMgr;
  std::unique_ptr<PhysicsWorld> physicsWorld{};

  explicit Cell(BaseId baseId, std::unique_ptr<PhysicsWorld> physicsWorld)
      : baseId(baseId),
        scnMgr(Ogre::Root::getSingleton().createSceneManager()),
        physicsWorld(std::move(physicsWorld)) {}

  ~Cell();
  Cell(const Cell &) = delete;
  Cell &operator=(const Cell &) = delete;
  Cell(Cell &&other) noexcept = default;
  Cell &operator=(Cell &&) noexcept = default;

  template<class Refr, class ...Res>
  void attach(Refr ref, gsl::not_null<Ogre::SceneNode *> node,
              std::tuple<const Res &...> resolvers);

 private:
  void setNodeTransform(Ogre::SceneNode *node,
                        const record::raw::REFRTransformation &transform);
};

template<>
struct ReifyRecordTrait<record::CELL> {
  using type = std::shared_ptr<Cell>;
  using resolvers = std::tuple<const Resolver<record::STAT> &,
                               const Resolver<record::DOOR> &,
                               const Resolver<record::LIGH> &,
                               Resolver<record::REFR_STAT, RefId> &,
                               Resolver<record::REFR_DOOR, RefId> &,
                               Resolver<record::REFR_LIGH, RefId> &,
                               const Resolver<record::CELL> &>;
};

/// Not a specialization because passing a Ogre::SceneManager doesn't make sense.
ReifyRecordTrait<record::CELL>::type
reifyRecord(const record::CELL &refRec,
            ReifyRecordTrait<record::CELL>::resolvers resolvers);

template<class Refr, class ...Res>
void Cell::attach(Refr ref, gsl::not_null<Ogre::SceneNode *> node,
                  std::tuple<const Res &...> resolvers) {
  auto entity{reifyRecord(ref, scnMgr, std::move(resolvers))};
  setNodeTransform(node, ref);
  attachAll(node,
            RefId{ref.mFormId},
            gsl::make_not_null(physicsWorld.get()),
            entity);
}

#endif // OPENOBLIVION_CELL_RESOLVER_HPP
