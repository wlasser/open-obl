#ifndef OPENOBLIVION_EXTERIOR_MANAGER_HPP
#define OPENOBLIVION_EXTERIOR_MANAGER_HPP

#include "record/formid.hpp"
#include "resolvers/cell_resolver.hpp"
#include "resolvers/wrld_resolver.hpp"
#include <boost/fiber/mutex.hpp>
#include <memory>
#include <set>
#include <vector>

namespace oo {

class ApplicationContext;
struct CellPacket;

/// Handles the loading and unloading of the exterior cells around a point.
///
/// ### The Cell (Un)Loading Process
///
/// At any point in time (that the player is in an exterior) the player is in
/// exactly one `oo::ExteriorCell`. Surrounding that *center cell* is a
/// *near neighborhood* of cells \f$G_0\f$ and a *far neighborhood* of cells
/// \f$F_0\f$, with \f$G_0 \subseteq F_0\f$. The cells in the near neighborhood
/// are fully loaded `oo::ExteriorCell`s---call such cells
/// *near-loaded*---whereas the cells in the far neighbourhood are loaded at a
/// lower LOD---call such cells *far-loaded*. Currently, a cell being far-loaded
/// means that only their `Ogre::Terrain` is loaded.
///
/// Suppose that all the cells in both neighborhoods have been loaded to their
/// correct level. When the player moves from one cell to another, there are new
/// near and far neighborhoods \f$G_1\f$ and \f$F_1\f$, respectively, which act
/// as targets for the sets of near-loaded and far-loaded cells. The act of
/// making the set of near-loaded cells equal to \f$G_1\f$ is encapsulated by
/// the function `reifyNearNeighborhood()`. Likewise, the act of making the set
/// of far-loaded cells equal to \f$F_1\f$ is encapsulated by
/// `reifyFarNeighborhood()`. Both of these functions are called by the public
/// method `reifyNeighborhood()`, which should be run as a job when the player
/// changes cell.
///
/// Ideally, `reifyNeighborhood()` would complete within a frame, though it is
/// unlikely to, and instead must be run as a background job. Because of this,
/// it could be that the player moves into another cell while
/// `reifyNeighborhood()` is executing, producing a new near neighborhood
/// \f$G_2\f$ and a new far neighborhood \f$F_2\f$. The most efficient approach
/// would be to keep whatever the current `reifyNeighborhood()` has done,
/// abandoning any further work, then start a new `reifyNeighbourhood()`
/// immediately that targets \f$F_2\f$ and \f$G_2\f$. A less efficient but
/// easier to implement method---the one that is used currently---is to finish
/// the current `reifyNeighborhood()` and run the new `reifyNeighborhood()`
/// as soon as it finishes. Either way, only one `reifyNeighborhood()` should
/// run at a time; this ensures that we always have well-defined near and far
/// neighborhood goals, and that when no `reifyNeighborhood()` is running we
/// are guaranteed to have met that goal. If multiple `reifyNeighborhood()`s
/// were allowed to run concurrently, we would have to worry about the ordering
/// of unload/load requests of the same cell.
///
/// Both `reifyNearNeighborhood()` and `reifyFarNeighborhood()` perform the same
/// essential steps:
///  - Find the set of target near/far neighbors, i.e. \f$G_1\f$ or \f$F_1\f$
///  - Find the subset of target cells that need to be unloaded,
///    i.e. \f$G_{01} := G_0 \setminus G_1\f$ and
///    \f$F_{01} := F_0 \backslash F_1\f$.
///  - Find the subset of target cells that need to be loaded,
///    i.e. \f$G_{10} := G_1 \setminus G_0\f$ and
///    \f$F_{10} := F_1 \backslash F_0\f$.
///  - For each cell in \f$G_{10}\f$ (or \f$F_{10}\f$) launch a job to reify
///    that cell, i.e. `reifyNearExteriorCell()` or `reifyFarExteriorCell()`.
///  - For each cell in \f$G_{01}\f$ (or \f$F_{01}\f$) launch a job to unload
///    that cell, i.e. `unloadNearExteriorCell()` or `unloadFarExteriorCell()`.
///  - Wait for all jobs to complete, then return.
///
/// Because only one `reifyNeighborhood()` can run at a time and the sets
/// \f$G_{10}\f$, \f$G_{01}\f$, \f$F_{10}\f$, and \f$F_{01}\f$ are computed
/// prior to any jobs being launched, these functions do not need to lock the
/// `mNearMutex` and `mFarMutex` mutexes.
class ExteriorManager {
  std::shared_ptr<oo::World> mWrld{};
  std::vector<std::shared_ptr<oo::ExteriorCell>> mNearCells{};
  /// Lock this during `reifyNeighbourhood` so that only one reify can occur
  /// at a time.
  boost::fibers::mutex mReifyMutex{};

  /// The set of all cells that are in a near-loaded state.
  /// \pre Lock `mNearMutex` before accessing.
  std::set<oo::BaseId> mNearLoaded{};
  /// Lock this before accessing `mNearLoaded`.
  boost::fibers::mutex mNearMutex{};

  /// The set of all cells that are in a far-loaded state.
  /// \pre Lock `mFarMutex`before accessing.
  std::set<oo::BaseId> mFarLoaded{};
  /// Lock this before accessing `mFarLoaded`.
  boost::fibers::mutex mFarMutex{};

  /// \post `mFarLoaded =` \f$F\f$
  void reifyFarNeighborhood(World::CellIndex centerCell,
                            ApplicationContext &ctx);

  /// \post `mFarLoaded.contains(cellId)`
  void reifyFarExteriorCell(oo::BaseId cellId, ApplicationContext &ctx);
  /// \post `!mFarLoaded.contains(cellId)`
  void unloadFarExteriorCell(oo::BaseId cellId, ApplicationContext &ctx);

  /// \post `mNearLoaded = ` \f$G\f$
  void reifyNearNeighborhood(World::CellIndex centerCell,
                             ApplicationContext &ctx);

  /// \post `mNearLoaded.contains(cellId)`
  void reifyNearExteriorCell(oo::BaseId cellId, ApplicationContext &ctx);
  /// \post `!mNearLoaded.contains(cellId)`
  void unloadNearExteriorCell(oo::BaseId cellId, ApplicationContext &ctx);

  auto getCellBaseResolvers(ApplicationContext &ctx) const {
    return oo::getResolvers<
        record::RACE, record::ACTI, record::DOOR, record::LIGH, record::MISC,
        record::STAT, record::NPC_>(ctx.getBaseResolvers());
  }

  auto getCellRefrResolvers(ApplicationContext &ctx) const {
    return oo::getRefrResolvers<
        record::REFR_ACTI, record::REFR_DOOR, record::REFR_LIGH,
        record::REFR_MISC, record::REFR_STAT,
        record::REFR_NPC_>(ctx.getRefrResolvers());
  }

  auto getCellMoreResolvers(ApplicationContext &ctx) const {
    return oo::getResolvers<record::LAND>(ctx.getBaseResolvers());
  }

  auto getCellResolvers(ApplicationContext &ctx) {
    return std::tuple_cat(getCellBaseResolvers(ctx), getCellRefrResolvers(ctx),
                          oo::getResolvers<record::CELL>(ctx.getBaseResolvers()));
  }

 public:
  explicit ExteriorManager(oo::CellPacket cellPacket) noexcept;
  ~ExteriorManager() = default;

  ExteriorManager(const ExteriorManager &) = delete;
  ExteriorManager &operator=(const ExteriorManager &) = delete;

  ExteriorManager(ExteriorManager &&) noexcept;
  ExteriorManager &operator=(ExteriorManager &&) noexcept;

  void reifyNeighborhood(World::CellIndex centerCell, ApplicationContext &ctx);
  /// \warning This is not fiber-safe.
  const std::vector<std::shared_ptr<oo::ExteriorCell>> &getNearCells() const noexcept;
  /// \warning This is not fiber-safe.
  const oo::World &getWorld() const noexcept;
  /// \warning This is not fiber-safe.
  oo::World &getWorld() noexcept;

  /// Set the visibility of all near cells.
  /// \warning This is not fiber-safe.
  void setVisible(bool visible);
};

} // namespace oo

#endif // OPENOBLIVION_EXTERIOR_MANAGER_HPP
