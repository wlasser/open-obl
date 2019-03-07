#include "cell_cache.hpp"
#include "game_settings.hpp"
#include "job/job.hpp"
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

  ctx.getLogger()->info("Looking for parent WRLD of CELL {} in "
                        "the WRLD cache...", cellId);

  const auto &worlds{ctx.getWorldCache()->worlds()};
  auto it{std::find_if(worlds.begin(), worlds.end(), [&](const auto &p) {
    boost::this_fiber::yield();
    return wrldRes.getCells(p->getBaseId())->contains(cellId);
  })};

  return it != worlds.end() ? std::optional{(*it)->getBaseId()} : std::nullopt;
}

std::optional<oo::BaseId>
LoadingMenuMode::getParentIdFromResolver(oo::BaseId cellId,
                                         ApplicationContext &ctx) const {
  auto &wrldRes{oo::getResolver<record::WRLD>(ctx.getBaseResolvers())};

  ctx.getLogger()->info("Looking for parent WRLD of CELL {} out of "
                        "the loaded WRLDs...", cellId);

  const auto &worlds{wrldRes.getWorlds()};
  auto it{std::find_if(worlds.begin(), worlds.end(), [&](auto wrldId) {
    boost::this_fiber::yield();
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

  ctx.getLogger()->info("Looking for parent WRLD of CELL {} out of "
                        "the unloaded WRLDs...", cellId);

  const auto &worlds{wrldRes.getWorlds()};
  auto it{std::find_if(worlds.begin(), worlds.end(), [&](auto wrldId) {
    // The worldspace is already loaded---and hence we do not need to search
    // it--iff getCells() returns a nonempty optional.
    if (wrldRes.getCells(wrldId)) return false;
    wrldRes.load(wrldId, resolvers);

    boost::this_fiber::yield();

    return wrldRes.getCells(wrldId)->contains(cellId);
  })};

  return it != worlds.end() ? std::optional{*it} : std::nullopt;
}

oo::BaseId LoadingMenuMode::getLoadedParentId(oo::BaseId cellId,
                                              ApplicationContext &ctx) const {
  if (auto opt{getParentIdFromCache(cellId, ctx)}; opt) return *opt;
  boost::this_fiber::yield();
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

  boost::this_fiber::yield();

  // Worldspace is not in cache, but is guaranteed to be loaded
  auto &wrldRes{oo::getResolver<record::WRLD>(ctx.getBaseResolvers())};
  auto resolvers{oo::getResolvers<record::CELL, record::WRLD, record::LTEX,
                                  record::WTHR, record::CLMT,
                                  record::LAND>(ctx.getBaseResolvers())};
  ctx.getLogger()->info("Reifying WRLD {}", wrldId);
  const auto wrldRec{*wrldRes.get(wrldId)};
  mWrld = oo::reifyRecord(wrldRec, std::move(resolvers));
  ctx.getWorldCache()->push_back(mWrld);
}

void LoadingMenuMode::reifyUncachedInteriorCell(oo::BaseId cellId,
                                                ApplicationContext &ctx) {
  auto &cellRes{oo::getResolver<record::CELL>(ctx.getBaseResolvers())};
  const record::CELL &cellRec{*cellRes.get(cellId)};

  ctx.getLogger()->info("Loading interior CELL {}", cellId);

  cellRes.load(cellId, getCellRefrResolvers(ctx),
               getCellBaseResolvers(ctx));

  boost::this_fiber::yield();

  ctx.getLogger()->info("Reifying interior CELL {}", cellId);

  mInteriorCell = std::static_pointer_cast<oo::InteriorCell>(
      oo::reifyRecord(cellRec, nullptr, nullptr, getCellResolvers(ctx)));
  ctx.getCellCache()->push_back(mInteriorCell);
  ctx.getLogger()->info("Loaded interior CELL {}", cellId);
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

  ctx.getLogger()->info("Loading exterior CELL {}", cellId);

  cellRes.load(cellId, getCellRefrResolvers(ctx), getCellBaseResolvers(ctx));

  boost::this_fiber::yield();

  ctx.getLogger()->info("Reifying exterior CELL {}", cellId);

  auto extPtr{std::dynamic_pointer_cast<oo::ExteriorCell>(
      oo::reifyRecord(cellRec, mWrld->getSceneManager(),
                      mWrld->getPhysicsWorld(), getCellResolvers(ctx)))};
  ctx.getCellCache()->push_back(extPtr);
  mExteriorCells.emplace_back(std::move(extPtr));

  boost::this_fiber::yield();

  ctx.getLogger()->info("Loading terrain of exterior CELL {}", cellId);

  mWrld->loadTerrain(*mExteriorCells.back());

  ctx.getLogger()->info("Loaded exterior CELL {}", cellId);
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

      boost::this_fiber::yield();
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

void LoadingMenuMode::startLoadJob(ApplicationContext &ctx) {
  mJc = std::make_shared<oo::JobCounter>(1);

  oo::RenderJobManager::runJob([&ctx, this]() {
    const auto cellId{mRequest.mCellId};
    auto &cellRes{oo::getResolver<record::CELL>(ctx.getBaseResolvers())};

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

        boost::this_fiber::yield();

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

          boost::this_fiber::yield();

          const World::CellIndex
              pos{cellRec.grid->data.x, cellRec.grid->data.y};
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

        boost::this_fiber::yield();

        // Cell exists and is an exterior cell.
        const auto cellRec{*cellRes.get(cellId)};
        const World::CellIndex pos{cellRec.grid->data.x, cellRec.grid->data.y};
        reifyNearNeighborhood(pos, ctx);
        return;
      }
    }
  }, mJc.get());
}

MenuMode<gui::MenuType::LoadingMenu>::MenuMode(ApplicationContext &ctx,
                                               oo::CellRequest request)
    : MenuModeBase<LoadingMenuMode>(ctx), mRequest(std::move(request)) {}

LoadingMenuMode::transition_t
LoadingMenuMode::handleEventImpl(ApplicationContext &ctx,
                                 const sdl::Event &/*event*/) {
  if (!mLoadStarted) {
    // TODO: Starting the job here is a hack to avoid the fact that we need to
    //       capture `this`, but the state might get moved after it's
    //       constructed.
    startLoadJob(ctx);
    mLoadStarted = true;
  }

  if (mJc->get() == 0) {
    ctx.getLogger()->info("Loading complete, changing state now");
    getMenuCtx()->getOverlay()->hide();
    return {false, oo::GameMode(ctx, CellPacket{
        std::move(mWrld), std::move(mInteriorCell), std::move(mExteriorCells),
        std::move(mRequest.mPlayerPosition),
        std::move(mRequest.mPlayerOrientation)
    })};
  }

  return {false, std::nullopt};
}

void LoadingMenuMode::updateImpl(ApplicationContext &/*ctx*/, float /*delta*/) {
  const float maximumProgress{getMenuCtx()->get_user<float>(4)};
  const float currentProgress{(getClock() / 10.0f) * maximumProgress};
  getMenuCtx()->set_user(3, std::min(currentProgress, maximumProgress));
}

} // namespace oo
