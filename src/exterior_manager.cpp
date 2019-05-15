#include "application_context.hpp"
#include "cell_cache.hpp"
#include "exterior_manager.hpp"
#include "job/job.hpp"
#include <spdlog/fmt/ostr.h>
#include <mutex>

namespace oo {

ExteriorManager::ExteriorManager(oo::CellPacket cellPacket) noexcept {
  mWrld = std::move(cellPacket.mWrld);
  mNearCells = std::move(cellPacket.mExteriorCells);
  for (const auto &cell : mNearCells) {
    mNearLoaded.emplace(cell->getBaseId());
  }
}

ExteriorManager::ExteriorManager(ExteriorManager &&other) noexcept {
  std::scoped_lock lock{other.mNearMutex, other.mFarMutex, other.mReifyMutex};
  mWrld = std::exchange(other.mWrld, {});
  mNearCells = std::exchange(other.mNearCells, {});
  mNearLoaded = std::exchange(other.mNearLoaded, {});
  mFarLoaded = std::exchange(other.mFarLoaded, {});

  // TODO: Some jobs launched capture `this`, despite how terrible an idea that
  //       is. Obviously they'll break if the ExteriorManager is moved while
  //       they're active. As of writing that can't happen so long as no jobs
  //       are launched in GameMode's constructor, but I don't think that should
  //       really be relied on...
}

ExteriorManager &
ExteriorManager::operator=(ExteriorManager &&other) noexcept {
  if (this != &other) {
    std::scoped_lock lock{mNearMutex, mFarMutex, mReifyMutex,
                          other.mNearMutex, other.mFarMutex, other.mReifyMutex};
    mWrld = std::exchange(other.mWrld, {});
    mNearCells = std::exchange(other.mNearCells, {});
    mNearLoaded = std::exchange(other.mNearLoaded, {});
    mFarLoaded = std::exchange(other.mFarLoaded, {});
  }

  return *this;
}

const std::vector<std::shared_ptr<oo::ExteriorCell>> &
ExteriorManager::getNearCells() const noexcept {
  return mNearCells;
}

const oo::World &ExteriorManager::getWorld() const noexcept {
  return *mWrld.get();
}

oo::World &ExteriorManager::getWorld() noexcept {
  return *mWrld.get();
}

std::set<oo::BaseId>
ExteriorManager::getNeighborhood(oo::CellIndex center,
                                 unsigned int diameter,
                                 ApplicationContext &ctx) {
  const auto &wrldRes{oo::getResolver<record::WRLD>(ctx.getBaseResolvers())};
  auto nbrs{wrldRes.getNeighbourhood(mWrld->getBaseId(), center, diameter)};

  std::set<oo::BaseId> nbrsSet{};
  for (const auto &row : nbrs) {
    for (auto id : row) {
      nbrsSet.emplace(id);
    }
  }

  return nbrsSet;
}

void ExteriorManager::loadExteriorCell(const record::CELL &cellRec,
                                       ApplicationContext &ctx) {
  auto &cellRes{oo::getResolver<record::CELL>(ctx.getBaseResolvers())};
  const oo::BaseId cellId{cellRec.mFormId};
  const auto cellGrid{cellRec.grid->data};
  oo::CellIndex cellIndex{cellGrid.x, cellGrid.y};

  cellRes.load(cellId, getCellRefrResolvers(ctx), getCellBaseResolvers(ctx));
  const auto &refLocator{ctx.getPersistentReferenceLocator()};
  for (auto persistentRef : refLocator.getRecordsInCell(mWrld->getBaseId(),
                                                        cellIndex)) {
    cellRes.insertReferenceRecord(cellId, persistentRef);
  }
}

std::shared_ptr<oo::ExteriorCell>
ExteriorManager::reifyExteriorCell(const record::CELL &cellRec,
                                   ApplicationContext &ctx) {
  auto extPtr{std::static_pointer_cast<oo::ExteriorCell>(
      reifyRecord(cellRec, mWrld->getSceneManager().get(),
                  mWrld->getPhysicsWorld().get(), getCellResolvers(ctx)))};
  ctx.getCellCache()->push_back(extPtr);
  return extPtr;
}

void ExteriorManager::reifyFarExteriorCell(oo::BaseId cellId,
                                           ApplicationContext &ctx) {
  ctx.getLogger()->info("Loading terrain of far CELL {}", cellId);

  auto _ = gsl::finally([&]() {
    std::unique_lock lock{mFarMutex};
    mFarLoaded.emplace(cellId);
    ctx.getLogger()->info("Loaded terrain of far CELL {}", cellId);
  });

  if (auto[cellPtr, _]{ctx.getCellCache()->getCell(cellId)}; cellPtr) {
    // TODO: Show the terrain
    ctx.getCellCache()->promoteCell(cellId);
    ctx.getLogger()->info("Found CELL {} in cache, showing terrain (TODO)",
                          cellId);
    return;
  }

  oo::JobCounter jc{1};
  oo::RenderJobManager::runJob([this, cellId]() {
    std::unique_lock lock{mFarMutex};
    mWrld->loadTerrainOnly(cellId, /*async=*/true);
  }, &jc);
  jc.wait();
}

void ExteriorManager::unloadFarExteriorCell(oo::BaseId cellId,
                                            ApplicationContext &ctx) {
  ctx.getLogger()->info("Unloading terrain of far CELL {}", cellId);

  auto _ = gsl::finally([&]() {
    std::unique_lock lock{mFarMutex};
    auto it{std::find(mFarLoaded.begin(), mFarLoaded.end(), cellId)};
    mFarLoaded.erase(it);
    ctx.getLogger()->info("Unloaded terrain of far CELL {}", cellId);
  });

  if (auto[cellPtr, _]{ctx.getCellCache()->getCell(cellId)}; cellPtr) {
    // TODO: Hide the terrain
    ctx.getLogger()->info("Found CELL {} in cache, hiding terrain (TODO)",
                          cellId);
    return;
  }

  oo::JobCounter jc{1};
  oo::RenderJobManager::runJob([this, cellId]() {
    std::unique_lock lock{mFarMutex};
    mWrld->unloadTerrain(cellId);
  }, &jc);
  jc.wait();
}

void ExteriorManager::reifyNearExteriorCell(oo::BaseId cellId,
                                            ApplicationContext &ctx) {
  auto _ = gsl::finally([&]() {
    std::unique_lock lock{mNearMutex};
    mNearLoaded.emplace(cellId);
  });

  if (auto[cellPtr, _]{ctx.getCellCache()->getCell(cellId)}; cellPtr) {
    ctx.getLogger()->info("Found CELL {} in cache", cellId);
    ctx.getCellCache()->promoteCell(cellId);
    auto extPtr{std::static_pointer_cast<oo::ExteriorCell>(std::move(cellPtr))};

    oo::JobCounter jc{1};
    oo::RenderJobManager::runJob([ptr = extPtr.get()]() {
      ptr->setVisible(true);
    }, &jc);
    jc.wait();

    std::unique_lock lock{mNearMutex};
    mNearCells.emplace_back(std::move(extPtr));
    return;
  }

  auto &cellRes{oo::getResolver<record::CELL>(ctx.getBaseResolvers())};
  const record::CELL &cellRec{*cellRes.get(cellId)};

  ctx.getLogger()->info("Loading exterior CELL {}", cellId);
  loadExteriorCell(cellRec, ctx);
  boost::this_fiber::yield();

  ctx.getLogger()->info("Reifying exterior CELL {}", cellId);
  oo::JobCounter reifyDone{1};
  oo::RenderJobManager::runJob([this, &cellRec, &ctx]() {
    auto extPtr{this->reifyExteriorCell(cellRec, ctx)};
    std::unique_lock nearLock{mNearMutex};
    mNearCells.emplace_back(std::move(extPtr));

    // Take the far lock to avoid concurrent terrain loads from the far cells.
    std::unique_lock farLock{mFarMutex};
    mWrld->loadTerrain(*mNearCells.back());
  }, &reifyDone);
  reifyDone.wait();
  ctx.getLogger()->info("Loaded exterior CELL {}", cellId);
}

void ExteriorManager::unloadNearExteriorCell(oo::BaseId cellId,
                                             ApplicationContext &ctx) {
  std::unique_lock lock{mNearMutex};
  ctx.getLogger()->info("Unloading exterior CELL {}", cellId);

  auto _ = gsl::finally([&]() {
    auto it{std::find(mNearLoaded.begin(), mNearLoaded.end(), cellId)};
    mNearLoaded.erase(it);
  });

  // Find the loaded cell.
  auto p = [&](const auto &cellPtr) { return cellPtr->getBaseId() == cellId; };
  auto jt{std::find_if(mNearCells.begin(), mNearCells.end(), p)};

  // If it's cached, then need to hide the scene root and remove it from
  // mNearCells; this won't unload the terrain since it's still in the cache.
  if (auto[cellPtr, _]{ctx.getCellCache()->getCell(cellId)}; cellPtr) {
    oo::JobCounter jc{1};
    oo::RenderJobManager::runJob([this, jt, ptr = cellPtr.get()]() {
      ptr->setVisible(false);
      mNearCells.erase(jt);
    }, &jc);
    jc.wait();
    ctx.getLogger()->info("Found CELL {} in cache, hiding CELL", cellId);
    return;
  }

  // Otherwise, removing from mNearCells will delete the only remaining pointer
  // and unload the cell. This will call into Ogre via destructors so must be
  // run on the render thread.
  oo::JobCounter jc{1};
  oo::RenderJobManager::runJob([this, jt]() {
    mNearCells.erase(jt);
  }, &jc);
  jc.wait();
  ctx.getLogger()->info("Unloaded exterior CELL {}", cellId);
}

namespace {

// Helper function to save typing in loadFarNeighborhood etc.
std::set<oo::BaseId> set_difference(const std::set<oo::BaseId> &a,
                                    const std::set<oo::BaseId> &b) {
  std::set<oo::BaseId> out;
  std::set_difference(a.begin(), a.end(), b.begin(), b.end(),
                      std::inserter(out, out.end()));
  return out;
}

} // namespace

void ExteriorManager::reifyFarNeighborhood(oo::CellIndex center,
                                           ApplicationContext &ctx) {
  const auto &gameSettings{oo::GameSettings::getSingleton()};
  const auto diam{gameSettings.get<unsigned>("General.uGridDistantCount", 5)};

  // The set of far neighbours that we want to end up being loaded.
  const auto farNbrsNew{getNeighborhood(center, diam, ctx)};

  // Don't need to take the mFarMutex because only one reifyFarNeighbourhood
  // can occur at a time and therefore reifyFarExteriorCell is not executing.

  // Some of farToLoad might already be loaded, which is fine.
  const auto farToUnload{oo::set_difference(mFarLoaded, farNbrsNew)};
  const auto farToLoad{oo::set_difference(farNbrsNew, mFarLoaded)};

  oo::JobCounter farLoadJc{static_cast<int>(farToLoad.size())};
  for (auto id : farToLoad) {
    oo::JobManager::runJob([this, id, &ctx]() {
      this->reifyFarExteriorCell(id, ctx);
    }, &farLoadJc);
  }

  oo::JobCounter farUnloadJc{static_cast<int>(farToUnload.size())};
  for (auto id : farToUnload) {
    oo::JobManager::runJob([this, id, &ctx]() {
      this->unloadFarExteriorCell(id, ctx);
    }, &farUnloadJc);
  }

  farLoadJc.wait();
  farUnloadJc.wait();
}

void ExteriorManager::reifyNearNeighborhood(oo::CellIndex center,
                                            ApplicationContext &ctx) {
  const auto &gameSettings{oo::GameSettings::getSingleton()};
  const auto diam{gameSettings.get<unsigned int>("General.uGridsToLoad", 3)};

  // The set of near neighbours that we want to end up being loaded.
  const auto nearNbrsNew{getNeighborhood(center, diam, ctx)};

  // Don't need to take the mNearMutex because only one reifyNearNeighbourhood
  // can occur at a time and therefore reifyNearExteriorCell is not executing.

  // Some of nearToLoad might already be loaded, which is fine.
  const auto nearToUnload{oo::set_difference(mNearLoaded, nearNbrsNew)};
  const auto nearToLoad{oo::set_difference(nearNbrsNew, mNearLoaded)};

  oo::JobCounter nearLoadJc{static_cast<int>(nearToLoad.size())};
  for (auto id : nearToLoad) {
    oo::JobManager::runJob([this, id, &ctx]() {
      this->reifyNearExteriorCell(id, ctx);
    }, &nearLoadJc);
  }

  oo::JobCounter nearUnloadJc{static_cast<int>(nearToUnload.size())};
  for (auto id : nearToUnload) {
    oo::JobManager::runJob([this, id, &ctx]() {
      this->unloadNearExteriorCell(id, ctx);
    }, &nearUnloadJc);
  }

  nearLoadJc.wait();
  nearUnloadJc.wait();
}

void ExteriorManager::reifyNeighborhood(oo::CellIndex centerCell,
                                        ApplicationContext &ctx) {
  std::unique_lock lock{mReifyMutex};
  ctx.getLogger()->info("[{}]: reifyNeighborhood()",
                        boost::this_fiber::get_id());
  reifyNearNeighborhood(centerCell, ctx);
  reifyFarNeighborhood(centerCell, ctx);
}

void ExteriorManager::setVisible(bool visible) {
  for (auto &cell : mNearCells) cell->setVisible(visible);
}

} // namespace oo