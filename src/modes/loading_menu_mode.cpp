#include "cell_cache.hpp"
#include "game_settings.hpp"
#include "modes/game_mode.hpp"
#include "modes/loading_menu_mode.hpp"
#include "resolvers/cell_resolver.hpp"
#include "resolvers/wrld_resolver.hpp"
#include "world_cache.hpp"
#include <spdlog/fmt/ostr.h>

namespace oo {

std::optional<oo::BaseId>
LoadingMenuMode::getParentIdFromCache(oo::BaseId cellId,
                                      ApplicationContext &ctx) const {
  auto &wrldRes{oo::getResolver<record::WRLD>(ctx.getBaseResolvers())};

  const auto &worlds{ctx.getWorldCache()->worlds()};
  auto it{std::find_if(worlds.begin(), worlds.end(), [&](const auto &p) {
    return wrldRes.getCells(p->getBaseId())->contains(cellId);
  })};

  return it != worlds.end() ? std::optional{(*it)->getBaseId()} : std::nullopt;
}

std::optional<oo::BaseId>
LoadingMenuMode::getParentIdFromResolver(oo::BaseId cellId,
                                         ApplicationContext &ctx) const {
  auto &wrldRes{oo::getResolver<record::WRLD>(ctx.getBaseResolvers())};

  const auto &worlds{wrldRes.getWorlds()};
  auto it{std::find_if(worlds.begin(), worlds.end(), [&](auto wrldId) {
    auto cellOpt{wrldRes.getCells(wrldId)};
    return cellOpt && cellOpt->contains(cellId);
  })};

  return it != worlds.end() ? std::optional{*it} : std::nullopt;
}

std::optional<oo::BaseId>
LoadingMenuMode::getParentIdFromUnloaded(oo::BaseId cellId,
                                         ApplicationContext &ctx) const {
  auto &wrldRes{oo::getResolver<record::WRLD>(ctx.getBaseResolvers())};
  auto resolvers{oo::getResolvers<record::CELL>(ctx.getBaseResolvers())};

  const auto &worlds{wrldRes.getWorlds()};
  auto it{std::find_if(worlds.begin(), worlds.end(), [&](auto wrldId) {
    // The worldspace is already loaded---and hence we do not need to search
    // it--iff getCells() returns a nonempty optional.
    if (wrldRes.getCells(wrldId)) return false;
    wrldRes.load(wrldId, resolvers);

    return wrldRes.getCells(wrldId)->contains(cellId);
  })};

  return it != worlds.end() ? std::optional{*it} : std::nullopt;
}

oo::BaseId LoadingMenuMode::getLoadedParentId(oo::BaseId cellId,
                                              ApplicationContext &ctx) const {
  if (auto opt{getParentIdFromCache(cellId, ctx)}; opt) return *opt;
  if (auto opt{getParentIdFromResolver(cellId, ctx)}; opt) return *opt;

  ctx.getLogger()->error("Cell {} looks like an exterior cell but "
                         "does not belong to any worldspace", cellId);
  throw std::runtime_error("Exterior cell has no parent worldspace");
}

oo::BaseId LoadingMenuMode::getUnloadedParentId(oo::BaseId cellId,
                                                ApplicationContext &ctx) const {
  if (auto opt{getParentIdFromUnloaded(cellId, ctx)}; opt) return *opt;

  ctx.getLogger()->error("Cell {} does not exist", cellId);
  throw std::runtime_error("Cell does not exist");
}

void LoadingMenuMode::reifyWorldspace(oo::BaseId wrldId,
                                      ApplicationContext &ctx) {
  const auto &worlds{ctx.getWorldCache()->worlds()};
  auto it{std::find_if(worlds.begin(), worlds.end(), [&](const auto &p) {
    return p->getBaseId() == wrldId;
  })};

  if (it != worlds.end()) {
    mWrld = *it;
    return;
  }

  // Worldspace is not in cache, but is guaranteed to be loaded
  auto &wrldRes{oo::getResolver<record::WRLD>(ctx.getBaseResolvers())};
  auto resolvers{oo::getResolvers<record::CELL, record::WRLD, record::LTEX,
                                  record::WTHR, record::CLMT,
                                  record::LAND>(ctx.getBaseResolvers())};
  const auto wrldRec{*wrldRes.get(wrldId)};
  mWrld = oo::reifyRecord(wrldRec, std::move(resolvers));
  ctx.getWorldCache()->push_back(mWrld);
}

void LoadingMenuMode::reifyUncachedInteriorCell(oo::BaseId cellId,
                                                ApplicationContext &ctx) {
  auto &cellRes{oo::getResolver<record::CELL>(ctx.getBaseResolvers())};
  const record::CELL &cellRec{*cellRes.get(cellId)};

  cellRes.load(cellId, getCellRefrResolvers(ctx),
               getCellBaseResolvers(ctx));
  mInteriorCell = std::static_pointer_cast<oo::InteriorCell>(
      oo::reifyRecord(cellRec, nullptr, nullptr, getCellResolvers(ctx)));
  ctx.getCellCache()->push_back(mInteriorCell);
  ctx.getLogger()->info("Loaded cell {}", cellId);
}

void
LoadingMenuMode::reifyExteriorCell(oo::BaseId cellId, ApplicationContext &ctx) {
  auto &cellRes{oo::getResolver<record::CELL>(ctx.getBaseResolvers())};

  if (auto[cellPtr, isInterior]{ctx.getCellCache()->get(cellId)};
      cellPtr && isInterior) {
    auto extPtr{std::dynamic_pointer_cast<oo::ExteriorCell>(cellPtr)};
    mExteriorCells.emplace_back(std::move(extPtr));
    return;
  }

  const record::CELL &cellRec{*cellRes.get(cellId)};
  cellRes.load(cellId, getCellRefrResolvers(ctx), getCellBaseResolvers(ctx));
  auto extPtr{std::dynamic_pointer_cast<oo::ExteriorCell>(
      oo::reifyRecord(cellRec, mWrld->getSceneManager(),
                      mWrld->getPhysicsWorld(), getCellResolvers(ctx)))};
  ctx.getCellCache()->push_back(extPtr);
  mExteriorCells.emplace_back(std::move(extPtr));
  mWrld->loadTerrain(*mExteriorCells.back());

  ctx.getLogger()->info("Loaded cell {}", cellId);
}

void LoadingMenuMode::reifyNearNeighborhood(oo::World::CellIndex center,
                                            ApplicationContext &ctx) {
  const auto &gameSettings{oo::GameSettings::getSingleton()};
  const auto nearDiameter{gameSettings.get<unsigned int>(
      "General.uGridsToLoad", 3)};

  // If a near neighbour is in the cache then just copy the pointer, otherwise
  // reify the record and add it. Things are a lot simpler than in `GameMode`
  // because we don't have to worry about `mExteriorCells` already having cells
  // in it.
  auto neighbors{mWrld->getNeighbourhood(center, nearDiameter)};
  for (const auto &row : neighbors) {
    for (auto id : row) {
      if (auto[cellPtr, isInterior]{ctx.getCellCache()->get(id)};
          cellPtr && !isInterior) {
        mExteriorCells.emplace_back(
            std::static_pointer_cast<oo::ExteriorCell>(cellPtr));
      } else {
        reifyExteriorCell(id, ctx);
      }
    }
  }

  // We now need to ensure that as many of the `mExteriorCells` are cached as
  // possible. Note that traversing `mExteriorCells` and caching each cell iff
  // it is not already cached may displace an already cached cell, and naively
  // adding every cell whether it was cached or not may result in duplicates.
  // Instead, already cached cells should be promoted to the back.
  const auto &exteriors{ctx.getCellCache()->exteriors()};
  for (const auto &cellPtr : mExteriorCells) {
    // TODO: Improve the methods on CellCache to make this more efficient,
    //       currently this does way more searches than it needs to.
    auto begin{exteriors.begin()}, end{exteriors.end()};
    if (std::find(begin, end, cellPtr) != end) {
      ctx.getCellCache()->promote(cellPtr->getBaseId());
    } else {
      ctx.getCellCache()->push_back(cellPtr);
    }
  }
}

MenuMode<gui::MenuType::LoadingMenu>::MenuMode(ApplicationContext &ctx,
                                               oo::CellRequest request)
    : MenuModeBase<LoadingMenuMode>(ctx),
      mRequest(request) {
  const auto cellId{mRequest.mCellId};

  auto &cellRes{oo::getResolver<record::CELL>(ctx.getBaseResolvers())};

  // TODO: All this should be jobified, we obviously shouldn't be doing this
  //       in the constructor because it blocks the render thread, and then
  //       what's the point of a loading screen? :P

  // TODO: Update detach time of cells

  // First check cell cache.
  if (auto[cellPtr, isInterior]{ctx.getCellCache()->get(cellId)}; cellPtr) {
    // Cell exists in cache, is it interior or exterior?
    if (isInterior) {
      mInteriorCell = std::dynamic_pointer_cast<oo::InteriorCell>(cellPtr);
      ctx.getLogger()->info("Loaded cell {} from cache", cellId);
      return;
    } else {
      auto extPtr{std::dynamic_pointer_cast<oo::ExteriorCell>(cellPtr)};
      // Exterior cell is cached, but we need the worldspace to find out its
      // near neighbours, even if they're cached too.
      const oo::BaseId wrldId{getLoadedParentId(cellId, ctx)};
      reifyWorldspace(wrldId, ctx);

      const auto cellRec{*cellRes.get(cellId)};
      const World::CellIndex pos{cellRec.grid->data.x, cellRec.grid->data.y};
      reifyNearNeighborhood(pos, ctx);
      return;
    }
  } else {
    // Cell is not in cache, check if the cell resolver knows about it.
    if (cellRes.contains(cellId)) {
      // Cell is either interior or an exterior cell in a loaded worldspace.
      const record::CELL &cellRec{*cellRes.get(cellId)};

      if (cellRec.grid) {
        // Cell is exterior and in a worldspace that is already loaded, but we
        // need to find out which one. Most cell loads do not change the
        // worldspace, so check the cached worldspaces first.
        const oo::BaseId wrldId{getLoadedParentId(cellId, ctx)};
        reifyWorldspace(wrldId, ctx);

        const World::CellIndex pos{cellRec.grid->data.x, cellRec.grid->data.y};
        reifyNearNeighborhood(pos, ctx);
        return;
      } else {
        // Cell is interior, let's load it
        reifyUncachedInteriorCell(cellId, ctx);
        return;
      }
    } else {
      // Either the cell doesn't exist or it's an exterior cell that belongs to
      // a worldspace that hasn't been loaded yet. Load each of the worldspaces
      // in turn and look for the cell. Note that 'loading' here just builds a
      // list of child cells, which is pretty lightweight.
      const oo::BaseId wrldId{getUnloadedParentId(cellId, ctx)};
      reifyWorldspace(wrldId, ctx);

      // Cell exists and is an exterior cell.
      const auto cellRec{*cellRes.get(cellId)};
      const World::CellIndex pos{cellRec.grid->data.x, cellRec.grid->data.y};
      reifyNearNeighborhood(pos, ctx);
      return;
    }
  }
}

LoadingMenuMode::transition_t
LoadingMenuMode::handleEventImpl(ApplicationContext &ctx,
                                 const sdl::Event &/*event*/) {
  getMenuCtx()->getOverlay()->hide();
  return {false, oo::GameMode(ctx, CellPacket{
      std::move(mWrld), std::move(mInteriorCell), std::move(mExteriorCells),
      std::move(mRequest.mPlayerPosition),
      std::move(mRequest.mPlayerOrientation)
  })};
}

void LoadingMenuMode::updateImpl(ApplicationContext &/*ctx*/, float /*delta*/) {
  const float maximumProgress{getMenuCtx()->get_user<float>(4)};
  const float currentProgress{(getClock() / 10.0f) * maximumProgress};
  getMenuCtx()->set_user(3, std::min(currentProgress, maximumProgress));
}

} // namespace oo
