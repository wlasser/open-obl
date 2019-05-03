#ifndef OPENOBLIVION_LOADING_MENU_MODE_HPP
#define OPENOBLIVION_LOADING_MENU_MODE_HPP

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

/// \ingroup OpenOblivionModes
template<> struct MenuModeTransition<LoadingMenuMode> {
  using type = ModeTransition<LoadingMenuMode, GameMode>;
};

/// \ingroup OpenOblivionModes
template<> struct HideOverlayOnTransition<LoadingMenuMode> : std::true_type {};

/// Specialization of `oo::MenuMode` for the Loading Menu.
/// Load screens occur when moving from the title menu into a cell, or from one
/// cell into another. In general, the caller will not have any more information
/// about the nature of the cell to load than can be found by looking up its
/// id in the `oo::CellResolver`, so it is sufficient to describe the load
/// request by just the id of the cell to load. From that, we must work out
/// whether it is an interior or exterior cell, and if it is an exterior cell,
/// whether we also need to load the parent worldspace.
/// \ingroup OpenOblivionModes
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
  /// \pre The given worldspace must exist and already be loaded.
  // If `getParentIdFromCache` was called to get `wrldId` then obviously we
  // don't need to check the cache again, but it wouldn't save us much anyway.
  void reifyWorldspace(oo::BaseId wrldId, ApplicationContext &ctx);

  /// Reify the given interior cell, storing it in `mInteriorCell` and caching
  /// it.
  /// Unlike `reifyWorldspace`, this function does not expect the cell to be
  /// loaded, and will load it if necessary.
  /// \pre The given cell must exist, be an interior cell, and not already be
  ///      cached.
  /// \remark The reason for the difference in behaviour to `reifyWorldspace` is
  ///         simply that there are fewer cases where this needs to be done, and
  ///         when it does we already know if the cell is in cache or not.
  void reifyUncachedInteriorCell(oo::BaseId cellId, ApplicationContext &ctx);

  /// Reify the given exterior cell, storing it in `mExteriorCells` and caching
  /// it.
  /// \pre The given cell must exist, be an exterior cell, and `mWrld` must be
  ///      a reification of its parent worldspace.
  void reifyExteriorCell(oo::BaseId cellId, ApplicationContext &ctx);

  void reifyNearNeighborhood(oo::CellIndex center, ApplicationContext &ctx);

  void idLoadJob(oo::IdCellLocation loc, ApplicationContext &ctx);
  void positionLoadJob(oo::PositionCellLocation loc, ApplicationContext &ctx);

  void startLoadJob(ApplicationContext &ctx);

 public:
  explicit MenuMode<gui::MenuType::LoadingMenu>(ApplicationContext &ctx,
                                                oo::CellRequest request);

  ~MenuMode();
  MenuMode(const MenuMode &) = delete;
  LoadingMenuMode &operator=(const MenuMode &) = delete;

  MenuMode(MenuMode &&other) noexcept;
  MenuMode<gui::MenuType::LoadingMenu> &operator=(MenuMode &&other) noexcept;

  std::string getFilenameImpl() const {
    return "menus/loading_menu.xml";
  }

  LoadingMenuMode::transition_t
  handleEventImpl(ApplicationContext &ctx, const sdl::Event &event);

  void updateImpl(ApplicationContext &ctx, float delta);
};

} // namespace oo

#endif // OPENOBLIVION_LOADING_MENU_MODE_HPP
