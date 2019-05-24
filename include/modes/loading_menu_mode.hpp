#ifndef OPENOBL_LOADING_MENU_MODE_HPP
#define OPENOBL_LOADING_MENU_MODE_HPP

#include "cell_cache.hpp"
#include "job/job.hpp"
#include "modes/menu_mode.hpp"
#include "modes/menu_mode_base.hpp"
#include "resolvers/cell_resolver.hpp"
#include "resolvers/wrld_resolver.hpp"
#include <tuple>

namespace oo {

class GameMode;

class World;
class InteriorCell;
class ExteriorCell;

/// \ingroup OpenOBLModes
template<> struct MenuModeTransition<LoadingMenuMode> {
  using type = ModeTransition<LoadingMenuMode, GameMode>;
};

/// \ingroup OpenOBLModes
template<> struct HideOverlayOnTransition<LoadingMenuMode> : std::true_type {};

/// Specialization of `oo::MenuMode` for the Loading Menu.
/// Load screens occur when moving from the title menu into a cell, or from one
/// cell into another. In general, the caller will not have any more information
/// about the nature of the cell to load than can be found by looking up its
/// id in the `oo::CellResolver`, so it is sufficient to describe the load
/// request by just the id of the cell to load. From that, we must work out
/// whether it is an interior or exterior cell, and if it is an exterior cell,
/// whether we also need to load the parent worldspace.
/// \ingroup OpenOBLModes
template<> class MenuMode<gui::MenuType::LoadingMenu>
    : public MenuModeBase<LoadingMenuMode> {
 private:
  /// This mode gets its own scene manager as it is used between cell loads,
  /// when no scene manager is present.
  Ogre::SceneManager *mScnMgr{};
  /// Type of the `Ogre::SceneManager` to use for this mode.
  constexpr static const char *SCN_MGR_TYPE{"DefaultSceneManager"};
  /// Name of the `Ogre::SceneManager` to use for this mode.
  constexpr static const char *SCN_MGR_NAME{"__LoadingMenuSceneManager"};
  /// Camera to use for the scene.
  Ogre::Camera *mCamera{};
  /// Name of the `Ogre::Camera` to use for this mode.
  constexpr static const char *CAMERA_NAME{"__LoadingMenuCamera"};

  /// Worldspace owning the exterior cell being loaded, if any. The worldspace
  /// itself may also be being loaded.
  std::shared_ptr<World> mWrld{};
  /// Interior cell being loaded, if any.
  std::shared_ptr<InteriorCell> mInteriorCell{};
  /// Exterior cells being loaded, if any.
  std::vector<std::shared_ptr<ExteriorCell>> mExteriorCells{};
  /// The request telling us which cell to load and where to place the player.
  oo::CellRequest mRequest;
  /// Whether the load job has been started.
  bool mLoadStarted{false};
  /// Keep track of the loading progress. When this is zero, we're done.
  std::shared_ptr<oo::JobCounter> mJc{};

  auto getCellBaseResolvers(ApplicationContext &ctx) const {
    return oo::getResolvers<
        record::RACE, record::ACTI, record::CONT, record::DOOR, record::LIGH,
        record::MISC, record::STAT, record::FLOR, record::FURN,
        record::NPC_>(ctx.getBaseResolvers());
  }

  auto getCellRefrResolvers(ApplicationContext &ctx) const {
    return oo::getRefrResolvers<
        record::REFR_ACTI, record::REFR_CONT, record::REFR_DOOR,
        record::REFR_LIGH, record::REFR_MISC, record::REFR_STAT,
        record::REFR_FLOR, record::REFR_FURN,
        record::REFR_NPC_>(ctx.getRefrResolvers());
  }

  auto getCellMoreResolvers(ApplicationContext &ctx) const {
    return oo::getResolvers<record::LAND>(ctx.getBaseResolvers());
  }

  auto getCellResolvers(ApplicationContext &ctx) {
    return std::tuple_cat(getCellBaseResolvers(ctx), getCellRefrResolvers(ctx),
                          oo::getResolvers<record::CELL>(ctx.getBaseResolvers()));
  }

  /// Implementation method of `getLoadedParentId`, looks in the world cache.
  std::optional<oo::BaseId>
  getParentIdFromCache(oo::BaseId cellId, ApplicationContext &ctx) const;

  /// Implementation method of `getLoadedParentId`, looks in the world
  /// resolver's list of worlds, but only checks the loaded ones.
  std::optional<oo::BaseId>
  getParentIdFromResolver(oo::BaseId cellId, ApplicationContext &ctx) const;

  /// Implementation method of `getUnloadedParentId`, looks in the world
  /// resolver's list of worlds, but only checks the unloaded ones.
  std::optional<oo::BaseId>
  getParentIdFromUnloaded(oo::BaseId cellId, ApplicationContext &ctx) const;

  /// Find the formid of the parent worldspace of the given exterior cell,
  /// only checking worldspaces that are *already loaded*.
  /// This is intended to be called for cells which are not in the cache, but
  /// are known to the cell resolver.
  /// \throws std::runtime_error if no worldspace can be found that has the
  ///                            given cell as a child.
  oo::BaseId
  getLoadedParentId(oo::BaseId cellId, ApplicationContext &ctx) const;

  /// Find the formid of the parent worldspace of the given exterior cell,
  /// only checking worldspaces that are *not loaded*. If the parent worldspace
  /// is found, then that worldspace is guaranteed to be loaded when this
  /// function returns. The loaded status of every other worldspace that was
  /// not loaded before this function was called is left unspecified.
  /// \throws std::runtime_error if no worldspace can be found that has the
  ///                            given cell as a child.
  oo::BaseId
  getUnloadedParentId(oo::BaseId cellId, ApplicationContext &ctx) const;

  /// Reify the given worldspace, storing it in `mWrld` and caching it.
  /// If the worldspace is already cached, then no additional reification or
  /// caching takes place.
  /// \pre The given worldspace must exist.
  /// \pre The given worldspace must already be loaded.
  // If `getParentIdFromCache` was called to get `wrldId` then obviously we
  // don't need to check the cache again, but it wouldn't save us much anyway.
  void reifyWorldspace(oo::BaseId wrldId, ApplicationContext &ctx);

  /// Load the given interior cell. This only loads the cell via the cell
  /// resolver, no reification takes place.
  void loadInteriorCell(const record::CELL &cellRec, ApplicationContext &ctx);

  /// Reify the given interior cell, returning a pointer to it and caching it.
  /// \pre The given cell must exist.
  /// \pre The given cell must be an interior cell.
  /// \pre The given cell must not already be cached.
  std::shared_ptr<oo::InteriorCell>
  reifyInteriorCell(const record::CELL &cellRec, ApplicationContext &ctx);

  /// Load and reify the given interior cell, storing it in `mInteriorCell` and
  /// caching it.
  /// \pre The given cell must exist.
  /// \pre The given cell must be an interior cell.
  /// \pre The given cell must not already be cached.
  void reifyInteriorCell(oo::BaseId cellId, ApplicationContext &ctx);

  /// Load the given exterior cell. This only loads the cell via the cell
  /// resolver, no reification takes place.
  void loadExteriorCell(const record::CELL &cellRec, ApplicationContext &ctx);

  /// Reify the given exterior cell, returning a pointer to it and caching it.
  /// \pre The given cell must exist.
  /// \pre The given cell must be an exterior cell.
  /// \pre The given cell must not already be cached.
  std::shared_ptr<oo::ExteriorCell>
  reifyExteriorCell(const record::CELL &cellRec, ApplicationContext &ctx);

  /// Load and reify the given exterior cell, storing it in `mExteriorCells` and
  /// caching it.
  /// \pre The given cell must exist.
  /// \pre The given cell must be an exterior cell.
  /// \pre The given cell must not already be cached.
  /// \pre `mWrld` must be a reification of the given cell's parent worldspace.
  void reifyExteriorCell(oo::BaseId cellId, ApplicationContext &ctx);

  /// Ensure that every cell in `neighbors` is reified and stored in
  /// `mExteriorCells`. This will not perform any reification of cached exterior
  /// cells.
  /// \remark It is expected that `neighbors` be the near neighborhood.
  void
  reifyNearNeighborhood(oo::CellGridView neighbors, ApplicationContext &ctx);

  /// Ensure that as many loaded exterior cells are present in the cache as is
  /// possible, with every cached loaded cell occurring later in the cache than
  /// any unloaded cell.
  /// Specifically, this iterates over every cell in `mExteriorCells` in an
  /// unspecified order, promoting each cell to the end of the cache or adding
  /// it to the back if not already there.
  /// If the cache size is at least as large as the number of cells in the near
  /// neighbourhood, then after this function every cell in `mExteriorCells`
  /// will be in the cache.
  void updateCellCache(oo::CellCache &cellCache);

  /// Reify the near neighborhood of the given center cell and add as many of
  /// those cells to the cell cache as possible.
  /// Specifically, call
  /// `reifyNearNeighborhood(oo::CellGridView, ApplicationContext &)` on the
  /// near neighborhood of `center`, then call `updateCellCache`.
  void reifyNearNeighborhood(oo::CellIndex center, ApplicationContext &ctx);

  /// Set `mInteriorCell` to the given interior cell.
  /// \pre `std::dynamic_cast<oo::InteriorCell *>(cellPtr.get()) != nullptr`.
  /// \remark The type of `cellPtr` was chosen to avoid a lengthy cast at the
  ///         callsite when the type is nonetheless known, such as when
  ///         `cellPtr` is obtained by a call to `oo::CellCache::getCell`.
  void setInteriorCell(std::shared_ptr<oo::Cell> cellPtr,
                       ApplicationContext &ctx);

  /// Set `mExteriorCells` to the near neighborhood of the given cell.
  /// This will load and reify all cells and the parent worldspace as needed;
  /// of course the center cell is reified, but the neighbors need not be.
  /// \pre `std::dynamic_cast<oo::ExteriorCell *>(cellPtr.get()) != nullptr`.
  /// \remark The type of `cellPtr` was chosen to avoid a length cast at the
  ///         callsite when the type is nonetheless known, such as when
  ///         `cellPtr` is obtained by a call to `oo::CellCache::getCell`.
  void setExteriorCells(std::shared_ptr<oo::Cell> cellPtr,
                        ApplicationContext &ctx);

  /// Implementation of a render job that deduces the type (interior/exterior)
  /// of the cell with the given formid and loads it and its near neighbourhood,
  /// as appropriate.
  /// \post If `loc` refers to an interior cell, then `mInteriorCell` shall be
  ///       a reification of that cell.
  /// \post If `loc` refers to an exterior cell, then `mExteriorCells` shall
  ///       be a reification of the near neighborhood of that cell, and `mWrld`
  ///       shall be a reification of the cell's parent worldspace.
  void idLoadJob(oo::IdCellLocation loc, ApplicationContext &ctx);

  /// Implementation of a render job that loads the exterior cell at the given
  /// position and its near neighborhood.
  /// \post `mExteriorCells` shall be a reification of the near neighborhood of
  ///       the referred to cell, and `mWrld` shall be a reification of the
  ///       referred cell's parent worldspace.
  void positionLoadJob(oo::PositionCellLocation loc, ApplicationContext &ctx);

  /// Start a render job calling `idLoadJob` or `positionLoadJob` depending on
  /// the held alternative of `mRequest.mLocation`.
  /// \post `mJc` shall be zero when the launched job completes.
  void startLoadJob(ApplicationContext &ctx);

  /// Set the music type according to the loaded cell or worldspace.
  /// If an interior cell was loaded and that cell does not have a
  /// `record::XCMT`, then the music type will be set to `MusicType::Default`.
  /// If an exterior cell was loaded and the parent worldspace of that cell
  /// does not have a `record::SNAM_WRLD`, then the music type will be set to
  /// `MusicType::Default`.
  /// \pre `mInteriorCell` is a reification of an interior cell or
  ///      `mWrld` is a reification of a worldspace.
  void setMusicType(ApplicationContext &ctx);

 public:
  explicit MenuMode(ApplicationContext &ctx, oo::CellRequest request);

  ~MenuMode();
  MenuMode(const MenuMode &) = delete;
  LoadingMenuMode &operator=(const MenuMode &) = delete;

  MenuMode(MenuMode &&other) noexcept;
  LoadingMenuMode &operator=(MenuMode &&other) noexcept;

  std::string getFilenameImpl() const {
    return "menus/loading_menu.xml";
  }

  LoadingMenuMode::transition_t
  handleEventImpl(ApplicationContext &ctx, const sdl::Event &event);

  void updateImpl(ApplicationContext &ctx, float delta);
};

} // namespace oo

#endif // OPENOBL_LOADING_MENU_MODE_HPP
