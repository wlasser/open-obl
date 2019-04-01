#ifndef OPENOBLIVION_CELL_CACHE_HPP
#define OPENOBLIVION_CELL_CACHE_HPP

#include "wrld.hpp"
#include <boost/circular_buffer.hpp>
#include <boost/fiber/mutex.hpp>
#include <OgreQuaternion.h>
#include <OgreVector3.h>
#include <memory>

namespace oo {

class Cell;
class InteriorCell;
class ExteriorCell;
class World;

class CellCache {
 public:
  using InteriorPtr = std::shared_ptr<InteriorCell>;
  using ExteriorPtr = std::shared_ptr<ExteriorCell>;
  using CellPtr = std::shared_ptr<Cell>;
  using WorldPtr = std::shared_ptr<World>;

  struct GetResult {
    CellPtr cell;
    bool isInterior;
    explicit GetResult(CellPtr cell, bool isInterior)
        : cell(std::move(cell)), isInterior(isInterior) {}
  };

 private:
  /// Stores an `oo::World` and upon the destruction of the last copy of its
  /// stored pointer, removes all cached `oo::ExteriorCell`s that belong to the
  /// world .
  class InvalidationWrapper {
   private:
    /// Pointer to the owning `CellCache`. Is allowed to be null for move
    /// semantics.
    CellCache *mCache;
    /// `oo::World` stored. Is allowed to be null for move semantics.
    WorldPtr mPtr;

    /// If `mPtr `is non-null then find all `oo::ExteriorCell`s in `mCache`
    /// that belong to the pointed-to world and remove them. Calling this
    /// when the `InvalidationWrapper` leaves the cache ensures that---at least
    /// due to caching---no `oo::ExteriorCell` is left alive without a parent
    /// `oo::World`.
    /// \remark This is not fiber-safe, the caller is expected to already be
    ///         taking a lock. After all, this is only called when the world
    ///         buffer is being modified.
    bool invalidate() const noexcept;
   public:
    explicit InvalidationWrapper(CellCache *cache, WorldPtr ptr);
    /// Calls `invalidate()`.
    ~InvalidationWrapper();
    /// Any `oo::World` should be stored at most once in a cache, copying is
    /// not allowed.
    InvalidationWrapper(const InvalidationWrapper &) = delete;
    /// \copydoc InvalidationWrapper(const InvalidationWrapper &)
    InvalidationWrapper &operator=(const InvalidationWrapper &) = delete;
    /// Move constructing an `InvalidationWrapper` is fine, no additional
    /// action is required.
    InvalidationWrapper(InvalidationWrapper &&other) noexcept = default;
    /// Calls `invalidate()` then move-assigns.
    /// Each `InvalidationWrapper` is local to a single `CellCache`, so the
    /// only use of move-assignment would be to replace an existing
    /// `InvalidationWrapper` with one that owns a different `oo::World`. Since
    /// that destructs the currently owned `oo::World`, we need to invalidate.
    InvalidationWrapper &operator=(InvalidationWrapper &&other) noexcept;

    const WorldPtr &get() const noexcept;
  };

 public:
  using InteriorBuffer = boost::circular_buffer<InteriorPtr>;
  using ExteriorBuffer = boost::circular_buffer<ExteriorPtr>;
  using WorldBuffer = boost::circular_buffer<InvalidationWrapper>;

 private:
  InteriorBuffer mInteriors{};
  ExteriorBuffer mExteriors{};
  /// \remark The `WorldBuffer` must be destroyed *before* the `ExteriorBuffer`,
  ///         because when the `InvalidationWrapper`s are destroyed they
  ///         remove all owned cells in the `ExteriorBuffer`. If the
  ///         `ExteriorBuffer` has been destroyed then they can't do that. After
  ///         the `WorldBuffer` is destroyed the only `ExteriorCell`s left in
  ///         the `ExteriorBuffer` will be those owned by non-cached worlds,
  ///         which---because the `CellCache` outlasts all `World`s----should
  ///         all have been destroyed.
  WorldBuffer mWorlds{};
  mutable boost::fibers::mutex mMutex{};

 public:
  explicit CellCache(std::size_t interiorCapacity,
                     std::size_t exteriorCapacity,
                     std::size_t worldCapacity) :
      mInteriors(interiorCapacity),
      mExteriors(exteriorCapacity),
      mWorlds(worldCapacity) {}

  void push_back(const InteriorPtr &interiorCell);
  void push_back(const ExteriorPtr &exteriorCell);
  void push_back(const WorldPtr &world);

  /// Move the given cell to the back of its buffer, if it exists.
  void promoteCell(oo::BaseId id);

  /// Move the given worldspace to the back of its buffer, if it exists.
  void promoteWorld(oo::BaseId id);

  const InteriorBuffer &interiors() const;
  const ExteriorBuffer &exteriors() const;
  std::vector<WorldPtr> worlds() const;

  GetResult getCell(oo::BaseId id) const;
  WorldPtr getWorld(oo::BaseId id) const;
};

struct IdCellLocation {
  oo::BaseId mCellId{};
  explicit IdCellLocation(oo::BaseId cellId) noexcept : mCellId(cellId) {}
  IdCellLocation() noexcept = default;
};

struct PositionCellLocation {
  oo::BaseId mWrldId{};
  oo::CellIndex mCellPos{};
  explicit PositionCellLocation(oo::BaseId wrldId,
                                oo::CellIndex cellPos) noexcept
      : mWrldId(wrldId), mCellPos(cellPos) {}
  explicit PositionCellLocation() noexcept = default;
};

using CellLocation = std::variant<IdCellLocation, PositionCellLocation>;

/// Cell to load and where to place the player in it.
/// \todo This is temporary, and needs to be moved/replaced with a better system
struct CellRequest {
  CellLocation mLocation{std::in_place_type<IdCellLocation>};
  Ogre::Vector3 mPlayerPosition{};
  Ogre::Quaternion mPlayerOrientation{};

  explicit CellRequest(oo::BaseId cellId,
                       const Ogre::Vector3 &playerPosition = Ogre::Vector3::ZERO,
                       const Ogre::Quaternion &playerOrientation = Ogre::Quaternion::IDENTITY)
      : mLocation(std::in_place_type<IdCellLocation>, cellId),
        mPlayerPosition(playerPosition),
        mPlayerOrientation(playerOrientation) {}
  explicit CellRequest(oo::BaseId wrldId, oo::CellIndex cellPos,
                       const Ogre::Vector3 &playerPosition = Ogre::Vector3::ZERO,
                       const Ogre::Quaternion &playerOrientation = Ogre::Quaternion::IDENTITY)
      : mLocation(std::in_place_type<PositionCellLocation>, wrldId, cellPos),
        mPlayerPosition(playerPosition),
        mPlayerOrientation(playerOrientation) {}
};

struct CellPacket {
  std::shared_ptr<World> mWrld{};
  std::shared_ptr<InteriorCell> mInteriorCell{};
  std::vector<std::shared_ptr<ExteriorCell>> mExteriorCells{};

  Ogre::Vector3 mPlayerPosition{};
  Ogre::Quaternion mPlayerOrientation{};

  explicit CellPacket(std::shared_ptr<World> world,
                      std::shared_ptr<InteriorCell> interiorCell,
                      std::vector<std::shared_ptr<ExteriorCell>> exteriorCells,
                      const Ogre::Vector3 &playerPosition = Ogre::Vector3::ZERO,
                      const Ogre::Quaternion &playerOrientation = Ogre::Quaternion::IDENTITY)
      : mWrld(std::move(world)),
        mInteriorCell(std::move(interiorCell)),
        mExteriorCells(std::move(exteriorCells)),
        mPlayerPosition(playerPosition),
        mPlayerOrientation(playerOrientation) {}
};

} // namespace oo

#endif // OPENOBLIVION_CELL_CACHE_HPP
