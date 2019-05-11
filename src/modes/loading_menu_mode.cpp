#include "cell_cache.hpp"
#include "game_settings.hpp"
#include "job/job.hpp"
#include "modes/game_mode.hpp"
#include "modes/loading_menu_mode.hpp"
#include "resolvers/cell_resolver.hpp"
#include "resolvers/wrld_resolver.hpp"
#include <spdlog/fmt/ostr.h>

namespace oo {

std::optional<oo::BaseId>
LoadingMenuMode::getParentIdFromCache(oo::BaseId cellId,
                                      ApplicationContext &ctx) const {
  auto &wrldRes{oo::getResolver<record::WRLD>(ctx.getBaseResolvers())};

  ctx.getLogger()->info("Looking for parent WRLD of CELL {} in "
                        "the WRLD cache...", cellId);

  const auto worlds{ctx.getCellCache()->worlds()};
  auto it{std::find_if(worlds.begin(), worlds.end(), [&](const auto &p) {
    boost::this_fiber::yield();
    //C++20: return wrldRes.getCells(p->getBaseId())->contains(cellId);
    const auto cells{wrldRes.getCells(p->getBaseId())};
    return cells->find(cellId) != cells->end();
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
    //C++20: return cellOpt && cellOpt->contains(cellId);
    return cellOpt && cellOpt->find(cellId) != cellOpt->end();
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

    const auto cells{wrldRes.getCells(wrldId)};
    //C++20: return cells && cells->contains(cellId);
    return cells && cells->find(cellId) != cells->end();
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
  const auto worlds{ctx.getCellCache()->worlds()};
  const auto p = [&](const auto wrld) { return wrld->getBaseId() == wrldId; };
  if (auto it{std::find_if(worlds.begin(), worlds.end(), p)};
      it != worlds.end()) {
    mWrld = *it;
    return;
  }

  boost::this_fiber::yield();

  // Worldspace is not in cache, but is guaranteed to be loaded
  auto &wrldRes{oo::getResolver<record::WRLD>(ctx.getBaseResolvers())};
  auto resolvers{oo::getResolvers<record::CELL, record::WRLD, record::LTEX,
                                  record::WTHR, record::CLMT, record::LAND,
                                  record::WATR>(ctx.getBaseResolvers())};
  ctx.getLogger()->info("Reifying WRLD {}", wrldId);
  const auto wrldRec{*wrldRes.get(wrldId)};
  mWrld = oo::reifyRecord(wrldRec, std::move(resolvers));
  ctx.getCellCache()->push_back(mWrld);
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

  if (auto[cellPtr, isInterior]{ctx.getCellCache()->getCell(cellId)};
      cellPtr && !isInterior) {
    cellPtr->setVisible(true);
    auto extPtr{std::dynamic_pointer_cast<oo::ExteriorCell>(cellPtr)};
    mExteriorCells.emplace_back(std::move(extPtr));
    return;
  }

  const record::CELL &cellRec{*cellRes.get(cellId)};
  const auto cellGrid{cellRec.grid->data};
  oo::CellIndex cellIndex{cellGrid.x, cellGrid.y};

  ctx.getLogger()->info("Loading exterior CELL {}", cellId);

  cellRes.load(cellId, getCellRefrResolvers(ctx), getCellBaseResolvers(ctx));
  const auto &refLocator{ctx.getPersistentReferenceLocator()};
  for (auto persistentRef : refLocator.getRecordsInCell(cellIndex)) {
    cellRes.insertReferenceRecord(cellId, persistentRef);
  }

  boost::this_fiber::yield();

  ctx.getLogger()->info("Reifying exterior CELL {}", cellId);

  auto extPtr{std::dynamic_pointer_cast<oo::ExteriorCell>(
      oo::reifyRecord(cellRec, mWrld->getSceneManager().get(),
                      mWrld->getPhysicsWorld().get(), getCellResolvers(ctx)))};
  ctx.getCellCache()->push_back(extPtr);
  mExteriorCells.emplace_back(std::move(extPtr));

  boost::this_fiber::yield();

  ctx.getLogger()->info("Loading terrain of exterior CELL {}", cellId);

  mWrld->loadTerrain(*mExteriorCells.back());

  ctx.getLogger()->info("Loaded exterior CELL {}", cellId);
}

void LoadingMenuMode::reifyNearNeighborhood(oo::CellIndex centerCell,
                                            ApplicationContext &ctx) {
  const auto &gameSettings{oo::GameSettings::getSingleton()};
  const auto nearDiameter{gameSettings.get<unsigned int>(
      "General.uGridsToLoad", 3)};

  // If a near neighbour is in the cache then just copy the pointer, otherwise
  // reify the record and add it. Things are a lot simpler than in `GameMode`
  // because we don't have to worry about `mExteriorCells` already having cells
  // in it.
  const auto &wrldRes{oo::getResolver<record::WRLD>(ctx.getBaseResolvers())};
  auto neighbors{wrldRes.getNeighbourhood(mWrld->getBaseId(), centerCell,
                                          nearDiameter)};
  for (const auto &row : neighbors) {
    for (auto id : row) {
      if (auto[cellPtr, isInterior]{ctx.getCellCache()->getCell(id)};
          cellPtr && !isInterior) {
        cellPtr->setVisible(true);
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
      ctx.getCellCache()->promoteCell(cellPtr->getBaseId());
    } else {
      ctx.getCellCache()->push_back(cellPtr);
    }
  }
}

void LoadingMenuMode::idLoadJob(IdCellLocation loc, ApplicationContext &ctx) {
  const auto cellId{loc.mCellId};
  auto &cellRes{oo::getResolver<record::CELL>(ctx.getBaseResolvers())};

  // TODO: Update detach time of cells

  // First check cell cache.
  if (auto[cellPtr, isInterior]{ctx.getCellCache()->getCell(cellId)};
      cellPtr) {
    // Cell exists in cache, is it interior or exterior?
    cellPtr->setVisible(true);
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
      const oo::CellIndex pos{cellRec.grid->data.x, cellRec.grid->data.y};
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

        const oo::CellIndex pos{cellRec.grid->data.x, cellRec.grid->data.y};
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
      const oo::CellIndex pos{cellRec.grid->data.x, cellRec.grid->data.y};
      reifyNearNeighborhood(pos, ctx);
      return;
    }
  }
}

void LoadingMenuMode::positionLoadJob(oo::PositionCellLocation loc,
                                      ApplicationContext &ctx) {
  const auto wrldId{loc.mWrldId};
  const auto pos{loc.mCellPos};
  auto baseResolvers{ctx.getBaseResolvers()};
  auto &wrldRes{oo::getResolver<record::WRLD>(baseResolvers)};

  // TODO: Update detach time of cells

  auto cellIdOpt{wrldRes.getCell(wrldId, pos).or_else([&]() {
    wrldRes.load(wrldId, oo::getResolvers<record::CELL>(baseResolvers));
    return wrldRes.getCell(wrldId, pos);
  })};

  if (!cellIdOpt) {
    ctx.getLogger()->error("WRLD {} does not exist or does not have a CELL "
                           "at ({}, {})", wrldId, qvm::X(pos), qvm::Y(pos));
    throw std::runtime_error("WRLD or CELL at given position does not exist");
  }

  reifyWorldspace(wrldId, ctx);
  boost::this_fiber::yield();
  reifyNearNeighborhood(pos, ctx);
}

void LoadingMenuMode::startLoadJob(ApplicationContext &ctx) {
  mJc = std::make_shared<oo::JobCounter>(1);

  const auto &loc{mRequest.mLocation};
  if (std::holds_alternative<oo::IdCellLocation>(loc)) {
    oo::RenderJobManager::runJob([&ctx, loc, this]() {
      this->idLoadJob(std::get<oo::IdCellLocation>(loc), ctx);
    }, mJc.get());
  } else {
    oo::RenderJobManager::runJob([&ctx, loc, this]() {
      this->positionLoadJob(std::get<oo::PositionCellLocation>(loc), ctx);
    }, mJc.get());
  }
}

MenuMode<gui::MenuType::LoadingMenu>::MenuMode(ApplicationContext &ctx,
                                               oo::CellRequest request)
    : MenuModeBase<LoadingMenuMode>(ctx),
      mScnMgr{ctx.getRoot().createSceneManager(SCN_MGR_TYPE, SCN_MGR_NAME)},
      mCamera{mScnMgr->createCamera(CAMERA_NAME)},
      mRequest(std::move(request)) {
  mScnMgr->addRenderQueueListener(ctx.getImGuiManager());
  mScnMgr->addRenderQueueListener(ctx.getOverlaySystem());

  ctx.setCamera(gsl::make_not_null(mCamera));
}

MenuMode<gui::MenuType::LoadingMenu>::~MenuMode() {
  auto *root{Ogre::Root::getSingletonPtr()};
  if (root && mScnMgr) root->destroySceneManager(mScnMgr);
}

MenuMode<gui::MenuType::LoadingMenu>::MenuMode(MenuMode &&other) noexcept
    : MenuModeBase<LoadingMenuMode>(std::move(other)),
      mScnMgr(std::exchange(other.mScnMgr, nullptr)),
      mCamera(std::exchange(other.mCamera, nullptr)),
      mWrld(std::move(other.mWrld)),
      mInteriorCell(std::move(other.mInteriorCell)),
      mExteriorCells(std::move(other.mExteriorCells)),
      mRequest(std::move(other.mRequest)),
      mLoadStarted(other.mLoadStarted),
      mJc(std::move(other.mJc)) {
  // TODO: If mLoadStarted == true then this is a really bad idea because the
  //       launched jobs capture *this*. Do something better than telling the
  //       user they broke it.
  if (mLoadStarted && mJc->get() != 0) {
    spdlog::get(oo::LOG)->critical("Moved LoadingMenuMode while jobs were "
                                   "running!");
  }
}

MenuMode<gui::MenuType::LoadingMenu> &LoadingMenuMode::operator=(MenuMode &&other) noexcept {
  if (this != &other) {
    auto tmp{std::move(other)};
    std::swap(*this, tmp);
  }
  return *this;
}

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
    return {true, oo::GameMode(ctx, CellPacket{
        std::move(mWrld), std::move(mInteriorCell), std::move(mExteriorCells),
        mRequest.mPlayerPosition, mRequest.mPlayerOrientation
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
