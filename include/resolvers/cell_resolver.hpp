#ifndef OPENOBLIVION_CELL_RESOLVER_HPP
#define OPENOBLIVION_CELL_RESOLVER_HPP

#include "bullet/configuration.hpp"
#include "esp_coordinator.hpp"
#include "resolvers/acti_resolver.hpp"
#include "resolvers/door_resolver.hpp"
#include "resolvers/helpers.hpp"
#include "resolvers/light_resolver.hpp"
#include "resolvers/npc_resolver.hpp"
#include "resolvers/resolvers.hpp"
#include "resolvers/static_resolver.hpp"
#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>
#include <btBulletDynamicsCommon.h>
#include <tl/optional.hpp>
#include <OgreRoot.h>
#include <OgreSceneManager.h>
#include <memory>
#include <utility>
#include <vector>

namespace oo {

using CellResolver = Resolver<record::CELL>;

template<>
class Resolver<record::CELL> {
 private:
  struct Metadata {
    /// Time that the player most recently left the cell, in in-game hours.
    /// This is measured from the epoch.
    int mDetachTime{};
    /// Whether the cell is an exterior cell. Interior cells that have the
    /// `BehvaveLikeExterior` flag set do not count.
    bool mIsExterior{false};
    /// Accessors, in load order of mods that modify the contents of the cell.
    std::vector<oo::EspAccessor> mAccessors{};
    /// All reference records inside the cell.
    /// This includes both esp records and ess records.
    absl::flat_hash_set<RefId> mReferences{};
  };

  /// Holds a record with an immutable backup of the original.
  /// Unlike general record::Record, it is not possible to create a new
  /// record::CELL at runtime.
  using RecordEntry = std::pair<record::CELL, tl::optional<record::CELL>>;
  using WrappedRecordEntry = std::pair<RecordEntry, Metadata>;

  /// Record storage.
  absl::flat_hash_map<oo::BaseId, WrappedRecordEntry> mRecords{};

  /// Bullet configuration, for constructing physics worlds.
  const bullet::Configuration &mBulletConf;

  class CellVisitor;
 public:
  using RecordIterator = typename decltype(mRecords)::iterator;

  /// Insert a new record with the given accessor if one exists, otherwise
  /// replace the existing record and append the accessor to the accessor list.
  /// Optionally specify that this cell is an exterior cell, if it is being
  /// registered in the context of a WRLD.
  std::pair<RecordIterator, bool>
  insertOrAppend(oo::BaseId baseId,
                 const record::CELL &rec,
                 oo::EspAccessor accessor,
                 bool isExterior = false);

  /// The bullet::Configuration is necessary to construct cells.
  /// bulletConf should live for at least as long as this object.
  explicit Resolver<record::CELL>(const bullet::Configuration &bulletConf)
      : mBulletConf(bulletConf) {}

  /// Get the underlying bullet configuration used for creating physics worlds.
  const bullet::Configuration &getBulletConfiguration() const;

  /// Return a reference to the cell.
  tl::optional<const record::CELL &> get(oo::BaseId baseId) const;

  /// \overload get(oo::BaseId)
  tl::optional<record::CELL &> get(oo::BaseId baseId);

  /// Reset the detach time for a cell to the given time, in in-game hours, from
  /// the epoch.
  void setDetachTime(oo::BaseId baseId, int detachTime);

  /// Return the detach time for the given cell in in-game hours from the epoch.
  int getDetachTime(oo::BaseId baseId) const;

  /// Checks if there is a cell with the baseId
  bool contains(oo::BaseId baseId) const;

  using RefrResolverContext = RefrResolverTuple<
      record::REFR_STAT, record::REFR_DOOR, record::REFR_LIGH,
      record::REFR_ACTI, record::REFR_NPC_>;

  using BaseResolverContext = ResolverTuple<
      record::STAT, record::DOOR, record::LIGH, record::ACTI, record::NPC_,
      record::RACE>;

  /// Load all child references of a cell.
  void load(oo::BaseId baseId,
            RefrResolverContext refrCtx,
            BaseResolverContext baseCtx);

  /// Return the RefIds of all reference records in the cell.
  /// \warning This will return an empty optional if the cell has not been
  ///          loaded first with a call to load.
  tl::optional<const absl::flat_hash_set<RefId> &>
  getReferences(oo::BaseId baseId) const;
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
  void readRecord(oo::EspAccessor &accessor) {
    accessor.skipRecord();
  }

  template<> void readRecord<record::REFR>(oo::EspAccessor &accessor);
  template<> void readRecord<record::ACHR>(oo::EspAccessor &accessor);
};

class Cell {
 public:
  using PhysicsWorld = btDiscreteDynamicsWorld;

  virtual gsl::not_null<Ogre::SceneManager *> getSceneManager() const = 0;
  virtual gsl::not_null<PhysicsWorld *> getPhysicsWorld() const = 0;

  oo::BaseId getBaseId() const;
  std::string getName() const;
  void setName(std::string name);

  explicit Cell(oo::BaseId baseId, std::string name)
      : mBaseId(baseId), mName(std::move(name)) {}

  virtual ~Cell() = 0;
  Cell(const Cell &) = delete;
  Cell &operator=(const Cell &) = delete;
  Cell(Cell &&other) noexcept = default;
  Cell &operator=(Cell &&) noexcept = default;

  template<class Refr, class ...Res>
  void attach(Refr ref, gsl::not_null<Ogre::SceneNode *> node,
              std::tuple<const Res &...> resolvers);

 protected:
  void setNodeTransform(Ogre::SceneNode *node,
                        const record::raw::REFRTransformation &transform);
 private:
  oo::BaseId mBaseId{};
  std::string mName{};
};

inline Cell::~Cell() = default;

class InteriorCell : public Cell {
 public:
  gsl::not_null<Ogre::SceneManager *> getSceneManager() const override;
  gsl::not_null<PhysicsWorld *> getPhysicsWorld() const override;

  explicit InteriorCell(oo::BaseId baseId, std::string name,
                        std::unique_ptr<PhysicsWorld> physicsWorld);
  ~InteriorCell() override;
  InteriorCell(const InteriorCell &) = delete;
  InteriorCell &operator=(const InteriorCell &) = delete;
  InteriorCell(InteriorCell &&) noexcept = default;
  InteriorCell &operator=(InteriorCell &&) noexcept = default;

 private:
  gsl::not_null<gsl::owner<Ogre::SceneManager *>> mScnMgr;
  std::unique_ptr<PhysicsWorld> mPhysicsWorld;
};

class ExteriorCell : public Cell {
 public:
  gsl::not_null<Ogre::SceneManager *> getSceneManager() const override;
  gsl::not_null<PhysicsWorld *> getPhysicsWorld() const override;

  explicit ExteriorCell(oo::BaseId baseId, std::string name,
                        gsl::not_null<Ogre::SceneManager *> scnMgr,
                        gsl::not_null<PhysicsWorld *> physicsWorld);
 private:
  gsl::not_null<Ogre::SceneManager *> mScnMgr;
  gsl::not_null<PhysicsWorld *> mPhysicsWorld;
};

template<>
struct ReifyRecordTrait<record::CELL> {
  using type = std::shared_ptr<Cell>;
  using resolvers = decltype(std::tuple_cat(
      std::declval<Resolver<record::CELL>::BaseResolverContext>(),
      std::declval<Resolver<record::CELL>::RefrResolverContext>(),
      std::declval<std::tuple<const oo::Resolver<record::CELL> &>>()));
};

/// Not a specialization because passing an Ogre::SceneManager is only necessary
/// for exterior cells.
/// \remark Pass nullptr for the Ogre::SceneManager and btDiscreteDynamicsWorld
///         to create a SceneManager and physics world for the cell.
ReifyRecordTrait<record::CELL>::type
reifyRecord(const record::CELL &refRec,
            Ogre::SceneManager *scnMgr,
            btDiscreteDynamicsWorld *physicsWorld,
            ReifyRecordTrait<record::CELL>::resolvers resolvers);

/// Parts of reifyRecord common to interior and exterior cells.
ReifyRecordTrait<record::CELL>::type
populateCell(std::shared_ptr<oo::Cell> cell, const record::CELL &refRec,
             ReifyRecordTrait<record::CELL>::resolvers resolvers);

template<class Refr, class ...Res>
void Cell::attach(Refr ref, gsl::not_null<Ogre::SceneNode *> node,
                  std::tuple<const Res &...> resolvers) {
  auto entity{reifyRecord(ref, getSceneManager(), std::move(resolvers))};
  setNodeTransform(node, ref);
  oo::attachAll(node, oo::RefId{ref.mFormId}, getPhysicsWorld(), entity);
}

} // namespace oo

#endif // OPENOBLIVION_CELL_RESOLVER_HPP
