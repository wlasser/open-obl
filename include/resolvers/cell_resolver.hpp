#ifndef OPENOBL_CELL_RESOLVER_HPP
#define OPENOBL_CELL_RESOLVER_HPP

#include "bullet/configuration.hpp"
#include "esp_coordinator.hpp"
#include "resolvers/acti_resolver.hpp"
#include "resolvers/cont_resolver.hpp"
#include "resolvers/door_resolver.hpp"
#include "resolvers/flor_resolver.hpp"
#include "resolvers/furn_resolver.hpp"
#include "resolvers/helpers.hpp"
#include "resolvers/light_resolver.hpp"
#include "resolvers/misc_resolver.hpp"
#include "resolvers/npc_resolver.hpp"
#include "resolvers/resolvers.hpp"
#include "resolvers/static_resolver.hpp"
#include <boost/fiber/mutex.hpp>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include <tl/optional.hpp>
#include <OgreSceneManager.h>
#include <Terrain/OgreTerrain.h>
#include <memory>
#include <utility>
#include <vector>

namespace oo {

using CellResolver = Resolver<record::CELL, oo::BaseId>;

template<>
class Resolver<record::CELL, oo::BaseId> {
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
    std::unordered_set<RefId> mReferences{};
    /// An optional base id for a LAND record describing the terrain of this
    /// cell. Should be present if `mIsExterior` is true.
    tl::optional<oo::BaseId> mLandId{};
  };

  /// Holds a record with an immutable backup of the original.
  /// Unlike general record::Record, it is not possible to create a new
  /// record::CELL at runtime.
  using RecordEntry = std::pair<record::CELL, tl::optional<record::CELL>>;
  using WrappedRecordEntry = std::pair<RecordEntry, Metadata>;

  /// Record storage.
  std::unordered_map<oo::BaseId, WrappedRecordEntry> mRecords{};

  /// Record storage mutex.
  mutable boost::fibers::mutex mMtx{};

  /// Bullet configuration, for constructing physics worlds.
  const bullet::Configuration &mBulletConf;

  class CellVisitor;
  class CellTerrainVisitor;
 public:
  using RecordIterator = typename decltype(mRecords)::iterator;

  ~Resolver() = default;
  Resolver(const Resolver &) = delete;
  Resolver &operator=(const Resolver &) = delete;
  Resolver(Resolver &&) noexcept;
  Resolver &operator=(Resolver &&) noexcept;

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
  explicit
  Resolver<record::CELL, oo::BaseId>(const bullet::Configuration &bulletConf)
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
      record::REFR_ACTI, record::REFR_CONT, record::REFR_DOOR,
      record::REFR_LIGH, record::REFR_MISC, record::REFR_STAT,
      record::REFR_FLOR, record::REFR_FURN, record::REFR_NPC_>;

  using BaseResolverContext = ResolverTuple<
      record::RACE, record::ACTI, record::CONT, record::DOOR, record::LIGH,
      record::MISC, record::STAT, record::FLOR, record::FURN, record::NPC_>;

  using MoreResolverContext = std::tuple<oo::Resolver<record::LAND> &>;

  /// Load all child references of a cell.
  void load(oo::BaseId baseId,
            RefrResolverContext refrCtx,
            BaseResolverContext baseCtx);

  /// Load the LAND and PGRD children of a cell, if it has them.
  void loadTerrain(oo::BaseId baseId, MoreResolverContext moreCtx);

  /// Return the RefIds of all reference records in the cell.
  /// \warning This will return an empty optional if the cell has not been
  ///          loaded first with a call to load.
  tl::optional<const std::unordered_set<RefId> &>
  getReferences(oo::BaseId baseId) const;

  /// Return the BaseId of the LAND record describing the terrain geometry of
  /// the cell.
  /// \warning This will return an empty optional if the cell has not had its
  ///          terrain loaded first with a call to `loadTerrain`.
  tl::optional<oo::BaseId> getLandId(oo::BaseId baseId) const;

  /// Add a reference record to the given cell, if it exists.
  /// \todo This is needed to add persistent records to exterior cells, which
  ///       are not saved with the cell itself, but it seems like an inelegant
  ///       solution.
  void insertReferenceRecord(oo::BaseId cellId, oo::RefId refId);
};

class CellResolver::CellVisitor {
 public:
  using Metadata = CellResolver::Metadata;
  using RefrContext = CellResolver::RefrResolverContext;
  using BaseContext = CellResolver::BaseResolverContext;
 private:
  Metadata &mMeta;
  RefrContext mRefrCtx;
  BaseContext mBaseCtx;

 public:
  CellVisitor(Metadata &meta, RefrContext refrCtx, BaseContext baseCtx)
      : mMeta(meta),
        mRefrCtx(std::move(refrCtx)),
        mBaseCtx(std::move(baseCtx)) {}

  template<class R> void readRecord(oo::EspAccessor &accessor) {
    (void) mMeta; // Clang bug? Fix -Wunused-private-field
    accessor.skipRecord();
  }
};

// CWG 727
template<> void
CellResolver::CellVisitor::readRecord<record::REFR>(oo::EspAccessor &accessor);
template<> void
CellResolver::CellVisitor::readRecord<record::ACHR>(oo::EspAccessor &accessor);

class CellResolver::CellTerrainVisitor {
 public:
  using Metadata = CellResolver::Metadata;
  using MoreContext = CellResolver::MoreResolverContext;
 private:
  Metadata &mMeta;
  MoreContext mMoreCtx;

 public:
  CellTerrainVisitor(Metadata &meta, MoreContext moreCtx)
      : mMeta(meta), mMoreCtx(std::move(moreCtx)) {}

  template<class R> void readRecord(oo::EspAccessor &accessor) {
    (void) mMeta; // Clang bug? Fix -Wunused-private-field
    accessor.skipRecord();
  }

};

// CWG 727
template<> void
CellResolver::CellTerrainVisitor::readRecord<record::LAND>(oo::EspAccessor &accessor);

class Cell {
 public:
  using PhysicsWorld = btDiscreteDynamicsWorld;

  virtual gsl::not_null<Ogre::SceneManager *> getSceneManager() const = 0;
  virtual gsl::not_null<PhysicsWorld *> getPhysicsWorld() const = 0;
  virtual gsl::not_null<Ogre::SceneNode *> getRootSceneNode() const = 0;
  virtual std::vector<std::unique_ptr<oo::Character>> &getCharacters() = 0;

  oo::BaseId getBaseId() const;
  std::string getName() const;
  void setName(std::string name);

  /// Show or hide the contents of this cell.
  /// Due to the `oo::CellCache`, a `Cell` may still be alive but not in the
  /// current scene. This method shows/hides the root scene node of this cell
  /// and all its children, as well as adding/removing all the owned
  /// physics objects.
  void setVisible(bool visible);
  bool isVisible() const noexcept;

  explicit Cell(oo::BaseId baseId, std::string name)
      : mBaseId(baseId), mName(std::move(name)) {}

  virtual ~Cell() = 0;
  Cell(const Cell &) = delete;
  Cell &operator=(const Cell &) = delete;
  Cell(Cell &&other) noexcept = delete;
  Cell &operator=(Cell &&) noexcept = delete;

  template<class Refr, class ...Res>
  void attach(Refr ref, std::tuple<const Res &...> resolvers);

 protected:
  void setNodeTransform(gsl::not_null<Ogre::SceneNode *> node,
                        const record::raw::REFRTransformation &transform);
  void setNodeScale(gsl::not_null<Ogre::SceneNode *> node,
                    const record::raw::REFRScalable &scalable);

  void destroyMovableObjects(Ogre::SceneNode *root);

  void showNode(gsl::not_null<Ogre::SceneNode *> node);
  void hideNode(gsl::not_null<Ogre::SceneNode *> node);

 private:
  oo::BaseId mBaseId{};
  std::string mName{};
  bool mIsVisible{true};
  std::vector<std::unique_ptr<oo::Character>> mCharacters{};

  virtual void setVisibleImpl(bool visible);
};

inline Cell::~Cell() = default;

class InteriorCell : public Cell {
 public:
  gsl::not_null<Ogre::SceneManager *> getSceneManager() const override;
  gsl::not_null<PhysicsWorld *> getPhysicsWorld() const override;
  gsl::not_null<Ogre::SceneNode *> getRootSceneNode() const override;
  std::vector<std::unique_ptr<oo::Character>> &getCharacters() override;

  explicit InteriorCell(oo::BaseId baseId, std::string name,
                        std::unique_ptr<PhysicsWorld> physicsWorld);
  ~InteriorCell() override;
  InteriorCell(const InteriorCell &) = delete;
  InteriorCell &operator=(const InteriorCell &) = delete;
  InteriorCell(InteriorCell &&) = delete;
  InteriorCell &operator=(InteriorCell &&) = delete;

 private:
  gsl::not_null<gsl::owner<Ogre::SceneManager *>> mScnMgr;
  std::unique_ptr<PhysicsWorld> mPhysicsWorld;
  std::vector<std::unique_ptr<oo::Character>> mCharacters{};
};

class ExteriorCell : public Cell {
 public:
  gsl::not_null<Ogre::SceneManager *> getSceneManager() const override;
  gsl::not_null<PhysicsWorld *> getPhysicsWorld() const override;
  gsl::not_null<Ogre::SceneNode *> getRootSceneNode() const override;
  std::vector<std::unique_ptr<oo::Character>> &getCharacters() override;
  btCollisionObject *getCollisionObject() const;

  explicit ExteriorCell(oo::BaseId baseId, std::string name,
                        gsl::not_null<Ogre::SceneManager *> scnMgr,
                        gsl::not_null<PhysicsWorld *> physicsWorld);
  ~ExteriorCell() override;
  ExteriorCell(const ExteriorCell &) = delete;
  ExteriorCell &operator=(const ExteriorCell &) = delete;
  ExteriorCell(ExteriorCell &&) = delete;
  ExteriorCell &operator=(ExteriorCell &&) = delete;

  void setTerrain(std::array<Ogre::Terrain *, 4> terrain);
 private:
  gsl::not_null<Ogre::SceneManager *> mScnMgr;
  gsl::not_null<PhysicsWorld *> mPhysicsWorld;
  gsl::not_null<Ogre::SceneNode *> mRootSceneNode;
  /// Logically the Cell should own its Terrain, but because the Terrain of
  /// every Cell needs to be known for LOD purposes before the Cell is reified,
  /// the Terrain is instead owned by the parent worldspace and managed with a
  /// TerrainGroup. For terrain blending reasons, each Cell is actually split
  /// into four quadrants of terrain;
  std::array<Ogre::Terrain *, 4> mTerrain{};
  /// Stores the row-reversed terrain heights needed by Bullet.
  // Our heightmap has its rows in the reverse order to what Bullet wants;
  // we go 'bottom to top' and Bullet needs 'top to bottom'.
  std::array<float, 33u * 33u> mTerrainHeights{};
  std::unique_ptr<btCollisionObject> mTerrainCollisionObject;
  std::unique_ptr<btHeightfieldTerrainShape> mTerrainCollisionShape;

  std::vector<std::unique_ptr<oo::Character>> mCharacters{};

  /// Implementation detail of `setVisible` which adds/removes the terrain's
  /// collision objects. The terrain itself is not unloaded since that is the
  /// responsibility of the `World`.
  void setVisibleImpl(bool visible) override;
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
void Cell::attach(Refr ref, std::tuple<const Res &...> resolvers) {
  // TODO: Abstract away the type difference
  if constexpr (std::is_same_v<std::decay_t<Refr>, record::REFR_NPC_>) {
    auto charPtr{reifyRecord(ref, getSceneManager(), getPhysicsWorld(),
                             std::move(resolvers), getRootSceneNode())};
    if (!charPtr) return;

    const auto &data{ref.positionRotation.data};
    charPtr->getController().moveTo(oo::fromBSCoordinates(
        Ogre::Vector3{data.x, data.y, data.z}));
    charPtr->getController().setOrientation(oo::fromBSTaitBryan(
        Ogre::Radian(data.aX),
        Ogre::Radian(data.aY),
        Ogre::Radian(data.aZ)));
    getCharacters().emplace_back(std::move(charPtr));
  } else {
    // TODO: Support returning different types
    auto childNode{reifyRecord(ref, getSceneManager(), getPhysicsWorld(),
                               std::move(resolvers), getRootSceneNode())};
    if (!childNode) return;

    setNodeTransform(gsl::make_not_null(childNode), ref);
    setNodeScale(gsl::make_not_null(childNode), ref);
  }
}

} // namespace oo

#endif // OPENOBL_CELL_RESOLVER_HPP
