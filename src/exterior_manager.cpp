#include "application_context.hpp"
#include "cell_cache.hpp"
#include "exterior_manager.hpp"
#include "job/job.hpp"
#include <spdlog/fmt/ostr.h>
#include <mutex>

oo::ExteriorManager::ExteriorManager(oo::CellPacket cellPacket) noexcept {
  mWrld = std::move(cellPacket.mWrld);
  mNearCells = std::move(cellPacket.mExteriorCells);
  for (const auto &cell : mNearCells) {
    mNearLoaded.emplace(cell->getBaseId());
  }
}

oo::ExteriorManager::ExteriorManager(ExteriorManager &&other) noexcept {
  std::scoped_lock lock{other.mNearMutex, other.mFarMutex, other.mReifyMutex};
  mWrld = std::move(other.mWrld);
  mNearCells = std::move(other.mNearCells);
  mNearLoaded = std::move(other.mNearLoaded);
  mFarLoaded = std::move(other.mFarLoaded);

  // TODO: Some jobs launched capture `this`, despite how terrible an idea that
  //       is. Obviously they'll break if the ExteriorManager is moved while
  //       they're active. As of writing that can't happen so long as no jobs
  //       are launched in GameMode's constructor, but I don't think that should
  //       really be relied on...
}

oo::ExteriorManager &
oo::ExteriorManager::operator=(ExteriorManager &&other) noexcept {
  if (this != &other) {
    std::scoped_lock lock{mNearMutex, mFarMutex, mReifyMutex,
                          other.mNearMutex, other.mFarMutex, other.mReifyMutex};
    mWrld = std::move(other.mWrld);
    mNearCells = std::move(other.mNearCells);
    mNearLoaded = std::move(other.mNearLoaded);
    mFarLoaded = std::move(other.mFarLoaded);
  }

  return *this;
}

const std::vector<std::shared_ptr<oo::ExteriorCell>> &
oo::ExteriorManager::getNearCells() const noexcept {
  return mNearCells;
}

const oo::World &oo::ExteriorManager::getWorld() const noexcept {
  return *mWrld.get();
}

oo::World &oo::ExteriorManager::getWorld() noexcept {
  return *mWrld.get();
}

void oo::ExteriorManager::reifyFarExteriorCell(oo::BaseId cellId,
                                               ApplicationContext &ctx) {
  ctx.getLogger()->info("[{}]: reifyFarExteriorCell({}) started",
                        boost::this_fiber::get_id(), cellId);

  auto _ = gsl::finally([&]() {
    std::unique_lock lock{mFarMutex};
    mFarLoaded.emplace(cellId);
    ctx.getLogger()->info("[{}]: reifyFarExteriorCell({}) finished",
                          boost::this_fiber::get_id(), cellId);
  });

  // By construction, this cell is not already loaded.

  // If it's cached, promote it in the cache and show the terrain.
  if (auto[cellPtr, isInterior]{ctx.getCellCache()->get(cellId)};
      cellPtr && !isInterior) {
    // TODO: Show the terrain
    ctx.getCellCache()->promote(cellId);
    ctx.getLogger()->info("[{}]: Loaded cell {} terrain from cache",
                          boost::this_fiber::get_id(), cellId);
    return;
  }

  // Otherwise need to actually load
  oo::JobCounter terrainLoadDone{1};
  oo::RenderJobManager::runJob([this, cellId]() {
    std::unique_lock lock{mFarMutex};
    mWrld->loadTerrainOnly(cellId, /*async=*/true);
  }, &terrainLoadDone);
  terrainLoadDone.wait();
}

void oo::ExteriorManager::unloadFarExteriorCell(oo::BaseId cellId,
                                                ApplicationContext &ctx) {
  ctx.getLogger()->info("[{}]: unloadFarExteriorCell({}) started",
                        boost::this_fiber::get_id(), cellId);

  auto _ = gsl::finally([&]() {
    std::unique_lock lock{mFarMutex};
    auto it{std::find(mFarLoaded.begin(), mFarLoaded.end(), cellId)};
    mFarLoaded.erase(it);
    ctx.getLogger()->info("[{}]: unloadFarExteriorCell({}) finished",
                          boost::this_fiber::get_id(), cellId);
  });

  // By construction, this cell is loaded.

  // If it's cached, then no unloading to do.
  if (auto[cellPtr, isInterior]{ctx.getCellCache()->get(cellId)};
      cellPtr && !isInterior) {
    // TODO: Hide the terrain
    ctx.getLogger()->info("[{}]: Cell is cached, hiding terrain [TODO]",
                          boost::this_fiber::get_id());
    return;
  }

  // Otherwise need to actually unload.
  std::unique_lock lock{mFarMutex};
  mWrld->unloadTerrain(cellId);
}

void oo::ExteriorManager::reifyFarNeighborhood(World::CellIndex centerCell,
                                               ApplicationContext &ctx) {
  const auto &gameSettings{oo::GameSettings::getSingleton()};
  const auto farDiameter{gameSettings.get<unsigned int>(
      "General.uGridDistantCount", 5)};

  // Get the set of far neighbours that we want to end up being loaded.
  auto farNbrsArray{mWrld->getNeighbourhood(centerCell, farDiameter)};
  std::set<oo::BaseId> farNbrsNew{};
  for (const auto &row : farNbrsArray) {
    for (auto id : row) {
      farNbrsNew.emplace(id);
    }
  }

  // Don't need to take the mFarMutex because only one reifyFarNeighbourhood
  // can occur at a time and therefore reifyFarExteriorCell is not executing.

  // Get the set of far neighbours that are currently loaded that we don't want
  // to be.
  std::set<oo::BaseId> farToUnload{};
  std::set_difference(mFarLoaded.begin(), mFarLoaded.end(),
                      farNbrsNew.begin(), farNbrsNew.end(),
                      std::inserter(farToUnload, farToUnload.end()));

  // Get the set of all far neighbours that need loading, and start jobs to load
  // them all. Some of these might already be loaded, which is fine.
  std::set<oo::BaseId> farToLoad{};
  std::set_difference(farNbrsNew.begin(), farNbrsNew.end(),
                      mFarLoaded.begin(), mFarLoaded.end(),
                      std::inserter(farToLoad, farToLoad.end()));

  oo::JobCounter farLoadJc{static_cast<int>(farToLoad.size())};
  for (auto id : farToLoad) {
    oo::JobManager::runJob([this, id, &ctx]() {
      this->reifyFarExteriorCell(id, ctx);
    }, &farLoadJc);
  }

  oo::JobCounter farUnloadJc{static_cast<int>(farToUnload.size())};
  for (auto id : farToUnload) {
    oo::RenderJobManager::runJob([this, id, &ctx]() {
      this->unloadFarExteriorCell(id, ctx);
    }, &farUnloadJc);
  }

  farLoadJc.wait();
  farUnloadJc.wait();
}

void oo::ExteriorManager::reifyNearExteriorCell(oo::BaseId cellId,
                                                ApplicationContext &ctx) {
  ctx.getLogger()->info("[{}]: reifyNearExteriorCell({}) started",
                        boost::this_fiber::get_id(), cellId);

  auto _ = gsl::finally([&]() {
    std::unique_lock lock{mNearMutex};
    mNearLoaded.emplace(cellId);
    ctx.getLogger()->info("[{}]: reifyNearExteriorCell({}) finished",
                          boost::this_fiber::get_id(), cellId);
  });

  // By construction, this cell is not already loaded.

  // If it's cached, promote it in the cache and make it visible.
  if (auto[cellPtr, isInterior]{ctx.getCellCache()->get(cellId)};
      cellPtr && !isInterior) {
    ctx.getCellCache()->promote(cellId);
    auto extCellPtr{std::static_pointer_cast<oo::ExteriorCell>(cellPtr)};
    extCellPtr->setVisible(true);
    std::unique_lock lock{mNearMutex};
    mNearCells.emplace_back(std::move(extCellPtr));
    ctx.getLogger()->info("[{}]: Loaded cell {} from cache",
                          boost::this_fiber::get_id(), cellId);
    return;
  }

  // Otherwise need to actually load.
  if (!mWrld) {
    throw std::runtime_error("Attempting to load an exterior cell "
                             "without a worldspace");
  }

  const auto wrldId{mWrld->getBaseId()};

  auto &cellRes{oo::getResolver<record::CELL>(ctx.getBaseResolvers())};
  auto &wrldRes{oo::getResolver<record::WRLD>(ctx.getBaseResolvers())};

  if (auto cells{wrldRes.getCells(wrldId)};
      !cells || !cells->contains(cellId)) {
    ctx.getLogger()->error("Cell {} does not exist in worldspace {}",
                           cellId, wrldId);
    throw std::runtime_error("Cell does not exist in worldspace");
  }

  const auto cellRec{cellRes.get(cellId)};

  cellRes.load(cellId, getCellRefrResolvers(ctx), getCellBaseResolvers(ctx));
  boost::this_fiber::yield();

  oo::JobCounter reifyDone{1};
  oo::RenderJobManager::runJob([this, cellRec, &ctx]() {
    std::unique_lock nearLock{mNearMutex};
    // TODO: We only need to lock mNearMutex for the emplace_back(), but some
    //       locking is required to prevent concurrent reifyRecords to avoid
    //       race conditions due to getPhysicsWorld() and getSceneManager().
    mNearCells.emplace_back(std::static_pointer_cast<oo::ExteriorCell>(
        reifyRecord(*cellRec,
                    mWrld->getSceneManager().get(),
                    mWrld->getPhysicsWorld().get(),
                    getCellResolvers(ctx))));
    // Note: we can't yield between reifyRecord and loadTerrain because if
    // control passes to the main render fiber then it will attempt to run
    // physics on the cell terrain, which doesn't exist yet.

    // Take the far lock to avoid concurrent terrain loads from the far cells.
    std::unique_lock farLock{mFarMutex};
    mWrld->loadTerrain(*mNearCells.back());
  }, &reifyDone);
  reifyDone.wait();

  oo::JobCounter cacheAddDone{1};
  oo::RenderJobManager::runJob([this, &ctx]() {
    std::unique_lock lock{mNearMutex};
    ctx.getCellCache()->push_back(mNearCells.back());
  }, &cacheAddDone);
  cacheAddDone.wait();
}

void oo::ExteriorManager::unloadNearExteriorCell(oo::BaseId cellId,
                                                 ApplicationContext &ctx) {
  std::unique_lock lock{mNearMutex};
  ctx.getLogger()->info("[{}]: unloadNearExteriorCell({}) started",
                        boost::this_fiber::get_id(), cellId);

  auto _ = gsl::finally([&]() {
    auto it{std::find(mNearLoaded.begin(), mNearLoaded.end(), cellId)};
    mNearLoaded.erase(it);
    ctx.getLogger()->info("[{}]: unloadNearExteriorCell({}) finished",
                          boost::this_fiber::get_id(), cellId);
  });

  // By construction, this cell is loaded.

  // Find the loaded cell
  auto jt{std::find_if(mNearCells.begin(), mNearCells.end(),
                       [&](const auto &cellPtr) {
                         return cellPtr->getBaseId() == cellId;
                       })};

  // If it's cached, then need to hide the scene root and remove it from
  // nearCells; this won't unload the terrain since it's still in the cache.
  if (auto[cellPtr, isInterior]{ctx.getCellCache()->get(cellId)};
      cellPtr && !isInterior) {
    static_cast<oo::ExteriorCell *>(cellPtr.get())->setVisible(false);
    mNearCells.erase(jt);
    ctx.getLogger()->info("[{}]: Cell is cached, hiding near cell",
                          boost::this_fiber::get_id());
    return;
  }

  // Otherwise, removing from nearCells will delete the only remaining pointer
  // and unload the cell.
  mNearCells.erase(jt);
}

void oo::ExteriorManager::reifyNearNeighborhood(World::CellIndex centerCell,
                                                ApplicationContext &ctx) {
  const auto &gameSettings{oo::GameSettings::getSingleton()};
  const auto nearDiameter{gameSettings.get<unsigned int>(
      "General.uGridsToLoad", 3)};

  // Get the set of near neighbours that we want to end up being loaded.
  auto nearNbrsArray{mWrld->getNeighbourhood(centerCell, nearDiameter)};
  std::set<oo::BaseId> nearNbrsNew{};
  for (const auto &row : nearNbrsArray) {
    for (auto id : row) {
      nearNbrsNew.emplace(id);
    }
  }

  // Don't need to take the mNearMutex because only one reifyNearNeighbourhood
  // can occur at a time and therefore reifyNearExteriorCell is not executing.

  // Get the set of near neighbours that are currently loaded but that we don't
  // want to be.
  std::set<oo::BaseId> nearToUnload{};
  std::set_difference(mNearLoaded.begin(), mNearLoaded.end(),
                      nearNbrsNew.begin(), nearNbrsNew.end(),
                      std::inserter(nearToUnload, nearToUnload.end()));

  // Get the set of all near neighbours that need loading and start jobs to
  // load them all. Some might already be loaded, which is fine.
  std::set<oo::BaseId> nearToLoad{};
  std::set_difference(nearNbrsNew.begin(), nearNbrsNew.end(),
                      mNearLoaded.begin(), mNearLoaded.end(),
                      std::inserter(nearToLoad, nearToLoad.end()));

  oo::JobCounter nearLoadJc{static_cast<int>(nearToLoad.size())};
  for (auto id : nearToLoad) {
    oo::JobManager::runJob([this, id, &ctx]() {
      this->reifyNearExteriorCell(id, ctx);
    }, &nearLoadJc);
  }

  oo::JobCounter nearUnloadJc{static_cast<int>(nearToUnload.size())};
  for (auto id : nearToUnload) {
    oo::RenderJobManager::runJob([this, id, &ctx]() {
      this->unloadNearExteriorCell(id, ctx);
    }, &nearUnloadJc);
  }

  nearLoadJc.wait();
  nearUnloadJc.wait();
}

void oo::ExteriorManager::reifyNeighborhood(World::CellIndex centerCell,
                                            ApplicationContext &ctx) {
  std::unique_lock lock{mReifyMutex};
  ctx.getLogger()->info("[{}]: reifyNeighborhood()",
                        boost::this_fiber::get_id());
  reifyNearNeighborhood(centerCell, ctx);
  reifyFarNeighborhood(centerCell, ctx);
}
